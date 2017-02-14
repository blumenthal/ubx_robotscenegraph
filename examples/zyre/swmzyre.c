#include <zyre.h>
#include <jansson.h>
#include <uuid/uuid.h>
#include <string.h>
#include "swmzyre.h"



void query_destroy (query_t **self_p) {
        assert (self_p);
        if(*self_p) {
            query_t *self = *self_p;
            destroy_message(self->msg);
            free (self);
            *self_p = NULL;
        }
}

void destroy_message(json_msg_t *msg) {
	free(msg->metamodel);
	free(msg->model);
	free(msg->type);
//	zstr_free(msg->payload);
	free(msg->payload);
	free(msg);
}

void destroy_component (component_t **self_p) {
    assert (self_p);
    if(*self_p) {
    	component_t *self = *self_p;
    	zactor_destroy (&self->communication_actor);
    	zclock_sleep (100);
    	zyre_stop (self->local);
		printf ("[%s] Stopping zyre node.\n", self->name);
		zclock_sleep (100);
		zyre_destroy (&self->local);
		printf ("[%s] Destroying component.\n", self->name);
        json_decref(self->config);
        //free memory of all items from the query list
        query_t *it;
        while(zlist_size (self->query_list) > 0){
        	it = zlist_pop (self->query_list);
        	query_destroy(&it);
        }
        zlist_destroy (&self->query_list);

        free (self);
        *self_p = NULL;
    }
}



query_t * query_new (const char *uid, const char *requester, json_msg_t *msg, zactor_t *loop) {
        query_t *self = (query_t *) zmalloc (sizeof (query_t));
        if (!self)
            return NULL;
        self->uid = uid;
        self->requester = requester;
        self->msg = msg;
        self->loop = loop;

        return self;
}

static void communication_actor (zsock_t *pipe, void *args)
{
	component_t *self = (component_t*) args;
	zpoller_t *poller =  zpoller_new (zyre_socket(self->local), pipe , NULL);
	zsock_signal (pipe, 0);

	while((!zsys_interrupted)&&(self->alive == 1)){
		//printf("[%s] Queries in queue: %d \n",self->name,zlist_size (self->query_list));
		void *which = zpoller_wait (poller, ZMQ_POLL_MSEC);
		if (which == zyre_socket(self->local)) {
			zmsg_t *msg = zmsg_recv (which );
			if (!msg) {
				printf("[%s] interrupted!\n", self->name);
			}
			//zmsg_print(msg); printf("msg end\n");
			char *event = zmsg_popstr (msg);
			if (streq (event, "ENTER")) {
				handle_enter (self, msg);
			} else if (streq (event, "EXIT")) {
				handle_exit (self, msg);
			} else if (streq (event, "SHOUT")) {
				char *rep;
				handle_shout (self, msg, &rep);
				if (rep) {
					printf("communication actor received reply: %s\n",rep);
					zstr_sendf(pipe,"%s",rep);
					printf("sent");
					zstr_free(&rep);
				}
			} else if (streq (event, "WHISPER")) {
				handle_whisper (self, msg);
			} else if (streq (event, "JOIN")) {
				handle_join (self, msg);
			} else if (streq (event, "EVASIVE")) {
				handle_evasive (self, msg);
			} else {
				zmsg_print(msg);
			}
			zstr_free (&event);
			zmsg_destroy (&msg);
		}
		else if (which == pipe) {
			zmsg_t *msg = zmsg_recv (which);
			if (!msg)
				break; //  Interrupted
			char *command = zmsg_popstr (msg);
			if (streq (command, "$TERM"))
				self->alive = 0;

			}
	}
	zpoller_destroy (&poller);
}

component_t* new_component(json_t *config) {
	component_t *self = (component_t *) zmalloc (sizeof (component_t));
    if (!self)
        return NULL;
    if (!config)
            return NULL;

    self->config = config;
    self->monitor = 0;

	self->name = json_string_value(json_object_get(config, "short-name"));
    if (!self->name) {
        destroy_component (&self);
        return NULL;
    }
	self->timeout = json_integer_value(json_object_get(config, "timeout"));
    if (self->timeout <= 0) {
    	destroy_component (&self);
        return NULL;
    }

	self->no_of_updates = json_integer_value(json_object_get(config, "no_of_updates"));
    if (self->no_of_updates <= 0) {
    	destroy_component (&self);
        return NULL;
    }

	self->no_of_queries = json_integer_value(json_object_get(config, "no_of_queries"));
    if (self->no_of_queries <= 0) {
    	destroy_component (&self);
        return NULL;
    }

	self->no_of_fcn_block_calls = json_integer_value(json_object_get(config, "no_of_fcn_block_calls"));
    if (self->no_of_fcn_block_calls <= 0) {
    	destroy_component (&self);
        return NULL;
    }
	//  Create local gossip node
	self->local = zyre_new (self->name);
    if (!self->local) {
    	destroy_component (&self);
        return NULL;
    }
	printf("[%s] my local UUID: %s\n", self->name, zyre_uuid(self->local));

	/* config is a JSON object */
	// set values for config file as zyre header.
	const char *key;
	json_t *value;
	json_object_foreach(config, key, value) {
		const char *header_value;
		if(json_is_string(value)) {
			header_value = json_string_value(value);
		} else {
			header_value = json_dumps(value, JSON_ENCODE_ANY);
		}
		printf("header key value pair\n");
		printf("%s %s\n",key,header_value);
		zyre_set_header(self->local, key, "%s", header_value);
	}

	//create a list to store queries...
	self->query_list = zlist_new();
	if ((!self->query_list)&&(zlist_size (self->query_list) == 0)) {
		destroy_component (&self);
		return NULL;
	}

	self->alive = 1; //will be used to quit program after answer to query is received

	int rc;
	if(!json_is_null(json_object_get(config, "gossip_endpoint"))) {
		rc = zyre_set_endpoint (self->local, "%s",json_string_value(json_object_get(config, "local_endpoint")));
		assert (rc == 0);

		//  Set up gossip network for this node
		zyre_gossip_connect (self->local, "%s", json_string_value(json_object_get(config, "gossip_endpoint")));
		printf("[%s] using gossip with gossip hub '%s' \n", self->name,json_string_value(json_object_get(config, "gossip_endpoint")));
	} else {
		printf("[%s] WARNING: no local gossip communication is set! \n", self->name);
	}
	rc = zyre_start (self->local);
	assert (rc == 0);
	///TODO: romove hardcoding of group name!
	self->localgroup = strdup("local");
	zyre_join (self->local, self->localgroup);
	//  Give time for them to connect
	zclock_sleep (1000);

	self->communication_actor = zactor_new (communication_actor, self);
	assert (self->communication_actor);
	///TODO: move to debug
	zstr_sendx (self->communication_actor, "VERBOSE", NULL);

	return self;
}

json_t * load_config_file(char* file) {
    json_error_t error;
    json_t * root;
    root = json_load_file(file, JSON_ENSURE_ASCII, &error);
	if(!root) {
		printf("Error parsing JSON file! line %d: %s\n", error.line, error.text);
    	return NULL;
    }
    printf("[%s] config file: %s\n", json_string_value(json_object_get(root, "short-name")), json_dumps(root, JSON_ENCODE_ANY));

    return root;
}

void register_monitor_callback(component_t* self, monitor_callback_t monitor) {
	self->monitor = monitor;
}

int decode_json(char* message, json_msg_t *result) {
	/**
	 * decodes a received msg to json_msg types
	 *
	 * @param received msg as char*
	 * @param json_msg_t* at which the result is stored
	 *
	 * @return returns 0 if successful and -1 if an error occurred
	 */
    json_t *root;
    json_error_t error;
    root = json_loads(message, 0, &error);

    if(!root) {
    	printf("Error parsing JSON string! line %d: %s\n", error.line, error.text);
    	return -1;
    }

    if (json_object_get(root, "metamodel")) {
    	result->metamodel = strdup(json_string_value(json_object_get(root, "metamodel")));
    } else {
    	printf("Error parsing JSON string! Does not conform to msg model.\n");
    	return -1;
    }
    if (json_object_get(root, "model")) {
		result->model = strdup(json_string_value(json_object_get(root, "model")));
	} else {
		printf("Error parsing JSON string! Does not conform to msg model.\n");
		return -1;
	}
    if (json_object_get(root, "type")) {
		result->type = strdup(json_string_value(json_object_get(root, "type")));
	} else {
		printf("Error parsing JSON string! Does not conform to msg model.\n");
		return -1;
	}
    if (json_object_get(root, "payload")) {
    	result->payload = strdup(json_dumps(json_object_get(root, "payload"), JSON_ENCODE_ANY));
	} else {
		printf("Error parsing JSON string! Does not conform to msg model.\n");
		return -1;
	}
    json_decref(root);
    return 0;
}

char* encode_json_message_from_file(component_t* self, char* message_file) {
    json_error_t error;
    json_t * pl;
    // create the payload, i.e., the query
    pl = json_load_file(message_file, JSON_ENSURE_ASCII, &error);
    printf("[%s] message file: %s\n", message_file, json_dumps(pl, JSON_ENCODE_ANY));
    if(!pl) {
   	printf("Error parsing JSON file! line %d: %s\n", error.line, error.text);
    	return NULL;
    }

    return encode_json_message(self, pl);
}

char* encode_json_message_from_string(component_t* self, char* message) {
    json_t *pl;
    json_error_t error;
    pl = json_loads(message, 0, &error);

    printf("[%s] message : %s\n", self->name , json_dumps(pl, JSON_ENCODE_ANY));
	if(!pl) {
	printf("Error parsing JSON file! line %d: %s\n", error.line, error.text);
		return NULL;
	}

    return encode_json_message(self, pl);
}

char* encode_json_message(component_t* self, json_t* message) {
    json_error_t error;
    json_t * pl = message;
    // create the payload, i.e., the query


    if(!pl) {
   	printf("Error parsing JSON file! line %d: %s\n", error.line, error.text);
    	return NULL;
    }

    // extract queryId
	if(json_object_get(pl,"queryId") == 0) { // no queryID in message, so we skip it here
		//printf("[%s] send_json_message: No queryId found, adding one.\n", self->name);
		zuuid_t *uuid = zuuid_new ();
		assert(uuid);
		const char* uuid_as_string = zuuid_str_canonical(uuid);
	    json_object_set_new(pl, "queryId", json_string(uuid_as_string));
	    free(uuid);
	}

	const char* query_id = json_string_value(json_object_get(pl,"queryId"));
	//printf("[%s] send_json_message: query_id = %s:\n", self->name, query_id);

	// pack it into the standard msg envelope
	json_t *env;
    env = json_object();
	json_object_set_new(env, "metamodel", json_string("SHERPA"));
	json_object_set_new(env, "model", json_string("RSGQuery"));
	json_object_set_new(env, "type", json_string("RSGQuery"));
	json_object_set(env, "payload", pl);

	// add it to the query list
	json_msg_t *msg = (json_msg_t *) zmalloc (sizeof (json_msg_t));
	msg->metamodel = strdup("SHERPA");
	msg->model = strdup("RSGQuery");
	msg->type = strdup("RSGQuery");
	msg->payload = json_dumps(pl, JSON_ENCODE_ANY);
	query_t * q = query_new(query_id, zyre_uuid(self->local), msg, NULL);
	zlist_append(self->query_list, q);

    char* ret = json_dumps(env, JSON_ENCODE_ANY);
	printf("[%s] send_json_message: message = %s:\n", self->name, ret);

	json_decref(env);

    return ret;
}

int shout_message(component_t* self, char* message) {
	 return zyre_shouts(self->local, self->localgroup, "%s", message);
}

char* wait_for_reply(component_t* self, char *msg, int timeout) {

	char* ret = NULL;
	if (timeout <= 0) {
		printf("[%s] Timeout has to be >0!\n",self->name);
		return ret;
	}

    // timestamp for timeout
    struct timespec ts = {0,0};
    struct timespec curr_time = {0,0};
    if (clock_gettime(CLOCK_MONOTONIC,&ts)) {
		printf("[%s] Could not assign time stamp!\n",self->name);
		return ret;
	}
    if (clock_gettime(CLOCK_MONOTONIC,&curr_time)) {
		printf("[%s] Could not assign time stamp!\n",self->name);
		return ret;
	}

    json_error_t error;
    json_t *sent_msg;
    sent_msg = json_loads(msg, 0, &error);
    // because of implementation inconsistencies between SWM and CM, we have to check for UID and queryId
    char *queryID = json_string_value(json_object_get(json_object_get(sent_msg,"payload"),"queryId"));
    if(!queryID) {
    	queryID = json_string_value(json_object_get(json_object_get(sent_msg,"payload"),"UID"));
    	if(!queryID) {
        	printf("[%s] Message has no queryID to wait for: %s\n", self->name, msg);
        	return ret;
    	}
    }

    //do this with poller?
    zsock_set_rcvtimeo (self->communication_actor, timeout); //set timeout for socket
    while (!zsys_interrupted){
    	ret = zstr_recv (self->communication_actor);
		if (ret){ //if ret is query we were waiting for in this thread
			//printf("[%s] wait_for_reply received answer %s:\n", self->name, ret);
			json_t *pl;
			pl = json_loads(ret, 0, &error);
			if(!pl) {
				printf("Error parsing JSON file! line %d: %s\n", error.line, error.text);
				return NULL;
			}
			//streq cannot take NULL, so check before
			char *received_queryID = json_string_value(json_object_get(json_object_get(sent_msg,"payload"),"queryId"));
			if(!received_queryID) {
				received_queryID = json_string_value(json_object_get(json_object_get(sent_msg,"payload"),"UID"));
				if(!received_queryID) {
					printf("[%s] Received message has no queryID: %s\n", self->name, msg);
					return NULL;
				}
			}
			if (streq(received_queryID,queryID)){
				printf("[%s] wait_for_reply received answer to query %s:\n", self->name, ret);
				json_decref(pl);
				break;
			}
			json_decref(pl);

		}
    	//if it is not the query we were waiting for in this thread, go back to recv if timeout has not happened yet
		if (!clock_gettime(CLOCK_MONOTONIC,&curr_time)) {
			// if timeout, stop component
			double curr_time_msec = curr_time.tv_sec*1.0e3 +curr_time.tv_nsec*1.0e-6;
			double ts_msec = ts.tv_sec*1.0e3 +ts.tv_nsec*1.0e-6;
			if (curr_time_msec - ts_msec > timeout) {
				printf("[%s] Timeout! No query answer received.\n",self->name);
				destroy_component(&self);
				break;
			}
		} else {
			printf ("[%s] could not get current time\n", self->name);
		}
    }
    json_decref(sent_msg);

    return ret;
}


char* send_query(component_t* self, char* query_type, json_t* query_params) {
	/**
	 * creates a query msg for the world model and adds it to the query list
	 *
	 * @param string query_type as string containing one of the available query types ["GET_NODES", "GET_NODE_ATTRIBUTES", "GET_NODE_PARENTS", "GET_GROUP_CHILDREN", "GET_ROOT_NODE", "GET_REMOTE_ROOT_NODES", "GET_TRANSFORM", "GET_GEOMETRY", "GET_CONNECTION_SOURCE_IDS", "GET_CONNECTION_TARGET_IDS"]
	 * @param json_t query_params json object containing all information required by the query; check the rsg-query-schema.json for details
	 *
	 * @return the string encoded JSON msg that can be sent directly via zyre. Must be freed by user! Returns NULL if wrong json types are passed in.
	 */

	// create the payload, i.e., the query
    json_t *pl;
    pl = json_object();
    json_object_set(pl, "@worldmodeltype", json_string("RSGQuery"));
    json_object_set(pl, "query", json_string(query_type));
	zuuid_t *uuid = zuuid_new ();
	assert(uuid);
    json_object_set(pl, "queryId", json_string(zuuid_str_canonical(uuid)));
	
	if (json_object_size(query_params)>0) {
		const char *key;
		json_t *value;
		json_object_foreach(query_params, key, value) {
			json_object_set(pl, key, value);
		}
	}

	// pack it into the standard msg envelope
	json_t *env;
    env = json_object();
	json_object_set(env, "metamodel", json_string("SHERPA"));
	json_object_set(env, "model", json_string("RSGQuery"));
	json_object_set(env, "type", json_string("RSGQuery"));
	json_object_set(env, "payload", pl);
	
	// add it to the query list
	json_msg_t *msg = (json_msg_t *) zmalloc (sizeof (json_msg_t));
	msg->metamodel = strdup("SHERPA");
	msg->model = strdup("RSGQuery");
	msg->type = strdup("RSGQuery");
	msg->payload = strdup(json_dumps(pl, JSON_ENCODE_ANY));
	query_t * q = query_new(zuuid_str_canonical(uuid), zyre_uuid(self->local), msg, NULL);
	zlist_append(self->query_list, q);

    char* ret = json_dumps(env, JSON_ENCODE_ANY);
	
	json_decref(env);
    json_decref(pl);
    return ret;
}

char* send_update(component_t* self, char* operation, json_t* update_params) {
	/**
	 * creates an update msg for the world model and adds it to the query list
	 *
	 * @param string operation as string containing one of the available operations ["CREATE","CREATE_REMOTE_ROOT_NODE","CREATE_PARENT","UPDATE_ATTRIBUTES","UPDATE_TRANSFORM","UPDATE_START","UPDATE_END","DELETE_NODE","DELETE_PARENT"]
	 * @param json_t update_params json object containing all information required by the update; check the rsg-update-schema.json for details
	 *
	 * @return the string encoded JSON msg that can be sent directly via zyre. Must be freed by user! Returns NULL if wrong json types are passed in.
	 */

	// create the payload, i.e., the query
    json_t *pl;
    pl = json_object();
    json_object_set(pl, "@worldmodeltype", json_string("RSGUpdate"));
    json_object_set(pl, "operation", json_string(operation));
    json_object_set(pl, "node", json_string(operation));
	zuuid_t *uuid = zuuid_new ();
	assert(uuid);
    json_object_set(pl, "queryId", json_string(zuuid_str_canonical(uuid)));

    if (!json_object_get(update_params,"node")) {
    	printf("[%s:send_update] No node object on parameters",self->name);
    	return NULL;
    }
	if (json_object_size(update_params)>0) {
		const char *key;
		json_t *value;
		json_object_foreach(update_params, key, value) {
			json_object_set(pl, key, value);
		}
	}
	// pack it into the standard msg envelope
	json_t *env;
    env = json_object();
	json_object_set(env, "metamodel", json_string("SHERPA"));
	json_object_set(env, "model", json_string("RSGQuery"));
	json_object_set(env, "type", json_string("RSGQuery"));
	json_object_set(env, "payload", pl);

	// add it to the query list
	json_msg_t *msg = (json_msg_t *) zmalloc (sizeof (json_msg_t));
	msg->metamodel = strdup("SHERPA");
	msg->model = strdup("RSGQuery");
	msg->type = strdup("RSGQuery");
	msg->payload = strdup(json_dumps(pl, JSON_ENCODE_ANY));
	query_t * q = query_new(zuuid_str_canonical(uuid), zyre_uuid(self->local), msg, NULL);
	zlist_append(self->query_list, q);

    char* ret = json_dumps(env, JSON_ENCODE_ANY);

	json_decref(env);
    json_decref(pl);
    return ret;
}

void handle_enter(component_t *self, zmsg_t *msg) {
	assert (zmsg_size(msg) == 4);
	char *peerid = zmsg_popstr (msg);
	char *name = zmsg_popstr (msg);
	zframe_t *headers_packed = zmsg_pop (msg);
	assert (headers_packed);
	zhash_t *headers = zhash_unpack (headers_packed);
	assert (headers);
	printf("header type %s\n",(char *) zhash_lookup (headers, "type"));
	char *address = zmsg_popstr (msg);
	printf ("[%s] ENTER %s %s <headers> %s\n", self->name, peerid, name, address);
	zstr_free(&peerid);
	zstr_free(&name);
	zframe_destroy(&headers_packed);
	zhash_destroy(&headers);
	zstr_free(&address);
}

void handle_exit(component_t *self, zmsg_t *msg) {
	assert (zmsg_size(msg) == 2);
	char *peerid = zmsg_popstr (msg);
	char *name = zmsg_popstr (msg);
	printf ("[%s] EXIT %s %s\n", self->name, peerid, name);
	zstr_free(&peerid);
	zstr_free(&name);
}

void handle_whisper (component_t *self, zmsg_t *msg) {
	assert (zmsg_size(msg) == 3);
	char *peerid = zmsg_popstr (msg);
	char *name = zmsg_popstr (msg);
	char *message = zmsg_popstr (msg);
	printf ("[%s] WHISPER %s %s %s\n", self->name, peerid, name, message);
	zstr_free(&peerid);
	zstr_free(&name);
	zstr_free(&message);
}

void handle_shout(component_t *self, zmsg_t *msg, char **rep) {
	assert (zmsg_size(msg) == 4);
	*rep = NULL;
	char *peerid = zmsg_popstr (msg);
	char *name = zmsg_popstr (msg);
	char *group = zmsg_popstr (msg);
	char *message = zmsg_popstr (msg);
	printf ("[%s] SHOUT %s %s %s %s\n", self->name, peerid, name, group, message);
	json_msg_t *result = (json_msg_t *) zmalloc (sizeof (json_msg_t));
	if (decode_json(message, result) == 0) {
		//printf ("[%s] message type %s\n", self->name, result->type);
		if (streq (result->type, "RSGUpdateResult")) {
			// load the payload as json
			json_t *payload;
			json_error_t error;
			payload= json_loads(result->payload,0,&error);
			if(!payload) {
				printf("Error parsing JSON send_remote! line %d: %s\n", error.line, error.text);
			} else {
				query_t *it = zlist_first(self->query_list);
				while (it != NULL) {
					if(json_object_get(payload,"queryId") == 0) { // no queryIt in message, so we skip it here
						printf("Skipping RSGUpdateResult message without queryId");
						break;
					}
					if (streq(it->uid,json_string_value(json_object_get(payload,"queryId")))) {
						printf("[%s] received answer to query %s:\n %s\n ", self->name,it->uid,result->payload);
						*rep = strdup(result->payload);
//						free(it->msg->payload);
						query_t *dummy = it;
						it = zlist_next(self->query_list);
						zlist_remove(self->query_list,dummy);
						query_destroy(&dummy);
					    json_decref(payload);
						break;
					} else {
						it = zlist_next(self->query_list);
					}
				}
			}
		} else if (streq (result->type, "RSGQueryResult")) {
			// load the payload as json
			json_t *payload;
			json_error_t error;
			payload= json_loads(result->payload,0,&error);
			if(!payload) {
				printf("Error parsing JSON send_remote! line %d: %s\n", error.line, error.text);
			} else {
				query_t *it = zlist_first(self->query_list);
				while (it != NULL) {
					if(json_object_get(payload,"queryId") == 0) { // no queryIt in message, so we skip it here
						printf("Skipping RSGQueryResult message without queryId\n");
						break;
					}
					if (streq(it->uid,json_string_value(json_object_get(payload,"queryId")))) {
						printf("[%s] received answer to query %s of type %s:\n Query:\n %s\n Result:\n %s \n", self->name,it->uid,result->type,it->msg->payload, result->payload);
						*rep = strdup(result->payload);
						query_t *dummy = it;
						it = zlist_next(self->query_list);
						zlist_remove(self->query_list,dummy);
						query_destroy(&dummy);
					    json_decref(payload);
						break;
					} else {
						it = zlist_next(self->query_list);
					}
				}
			}
		} else if (streq (result->type, "RSGFunctionBlockResult")) {
			// load the payload as json
			json_t *payload;
			json_error_t error;
			payload= json_loads(result->payload,0,&error);
			if(!payload) {
				printf("Error parsing JSON send_remote! line %d: %s\n", error.line, error.text);
			} else {
				query_t *it = zlist_first(self->query_list);
				while (it != NULL) {
					if(json_object_get(payload,"queryId") == 0) { // no queryIt in message, so we skip it here
						printf("Skipping RSGFunctionBlockResult message without queryId\n");
						break;
					}
					if (streq(it->uid,json_string_value(json_object_get(payload,"queryId")))) {
						printf("[%s] received answer to query %s of type %s:\n Query:\n %s\n Result:\n %s \n", self->name,it->uid,result->type,it->msg->payload, result->payload);
						*rep = strdup(result->payload);
						query_t *dummy = it;
						it = zlist_next(self->query_list);
						zlist_remove(self->query_list,dummy);
						query_destroy(&dummy);
					    json_decref(payload);
						break;
					} else {
						it = zlist_next(self->query_list);
					}
				}
			}
		} else if (streq (result->type, "RSGMonitor")) {
			// load the payload as json
			json_t *payload;
			json_error_t error;
			payload= json_loads(result->payload,0,&error);
			if(!payload) {
				printf("Error parsing JSON send_remote! line %d: %s\n", error.line, error.text);
			} else {

				printf("[%s] received a RSGMonitor message: %s \n", self->name, result->payload);
				char*monitor_msg = strdup(result->payload);

				/* In form potential listener, if it exists */
				if(self->monitor) {
					(*self->monitor)(monitor_msg);
				}

				free (monitor_msg);
				json_decref(payload);
			}
		} else if (streq (result->type, "mediator_uuid")) {
			// load the payload as json
			json_t *payload;
			json_error_t error;
			payload= json_loads(result->payload,0,&error);
			if(!payload) {
				printf("Error parsing JSON send_remote! line %d: %s\n", error.line, error.text);
			} else {
				query_t *it = zlist_first(self->query_list);
				while (it != NULL) {
					if(json_object_get(payload,"UID") == 0) { // no queryIt in message, so we skip it here
						printf("Skipping mediator_uuid message without queryId\n");
						break;
					}
					if (streq(it->uid,json_string_value(json_object_get(payload,"UID")))) {
						printf("[%s] received answer to query %s of type %s:\n Query:\n %s\n Result:\n %s \n", self->name,it->uid,result->type,it->msg->payload, result->payload);
						*rep = strdup(result->payload);
						query_t *dummy = it;
						it = zlist_next(self->query_list);
						zlist_remove(self->query_list,dummy);
						query_destroy(&dummy);
					    json_decref(payload);
						break;
					} else {
						it = zlist_next(self->query_list);
					}
				}
			}
		} else {
			printf("[%s] Unknown msg type!\n",self->name);
		}
	} else {
		printf ("[%s] message could not be decoded\n", self->name);
	}
	destroy_message(result);
	zstr_free(&peerid);
	zstr_free(&name);
	zstr_free(&group);
	zstr_free(&message);
}

void handle_join (component_t *self, zmsg_t *msg) {
	assert (zmsg_size(msg) == 3);
	char *peerid = zmsg_popstr (msg);
	char *name = zmsg_popstr (msg);
	char *group = zmsg_popstr (msg);
	printf ("[%s] JOIN %s %s %s\n", self->name, peerid, name, group);
	zstr_free(&peerid);
	zstr_free(&name);
	zstr_free(&group);
}

void handle_evasive (component_t *self, zmsg_t *msg) {
	assert (zmsg_size(msg) == 2);
	char *peerid = zmsg_popstr (msg);
	char *name = zmsg_popstr (msg);
	printf ("[%s] EVASIVE %s %s\n", self->name, peerid, name);
	zstr_free(&peerid);
	zstr_free(&name);
}


bool get_root_node_id(component_t *self, char** root_id) {
	assert(self);
	*root_id = NULL;
	char *msg;
	char *reply;

	// e.g.
//	{
//	  "@worldmodeltype": "RSGQuery",
//	  "query": "GET_ROOT_NODE"
//	}

	json_t *getRootNodeMsg = json_object();
	json_object_set_new(getRootNodeMsg, "@worldmodeltype", json_string("RSGQuery"));
	json_object_set_new(getRootNodeMsg, "query", json_string("GET_ROOT_NODE"));

	/* Send message and wait for reply */
	msg = encode_json_message(self, getRootNodeMsg);
	shout_message(self, msg);
	reply = wait_for_reply(self, msg, self->timeout);
	printf("#########################################\n");
	printf("[%s] Got reply for get_root_node_id: %s \n", self->name, reply);

	/* Parse reply */
    json_error_t error;
	json_t* rootNodeIdReply = json_loads(reply, 0, &error);
	free(msg);
	free(reply);
	json_t* rootNodeIdAsJSON = 0;

	json_t* querySuccessMsg = json_object_get(rootNodeIdReply, "querySuccess");
	bool querySuccess = false;
	char* dump = json_dumps(querySuccessMsg, JSON_ENCODE_ANY);
	printf("[%s] querySuccessMsg is: %s \n", self->name, dump);
	free(dump);
	if (querySuccessMsg) {
		querySuccess = json_is_true(querySuccessMsg);
	}

	json_t* rootIdMsg = json_object_get(rootNodeIdReply, "rootId");
	if (rootIdMsg) {
		*root_id = strdup(json_string_value(rootIdMsg));
		printf("[%s] get_root_node_id ID is: %s \n", self->name, *root_id);
	} else {
		querySuccess = false;
	}

	/* Clean up */
	json_decref(rootNodeIdReply);
	json_decref(getRootNodeMsg);

	return querySuccess;
}

bool get_gis_origin_id(component_t *self, char** origin_id) {
	assert(self);
//	*origin_id = NULL;
	return get_node_by_attribute(self, origin_id, "gis:origin", "wgs84");
}

bool get_observations_group_id(component_t *self, char** observations_id) {
	assert(self);

	/* Get root ID to restrict search to subgraph of local SWM */
	char* scope_id = 0;
	if (!get_root_node_id(self, &scope_id)) {
		printf("[%s] [ERROR] Cannot get root  Id \n", self->name);
		return false;
	}

	return get_node_by_attribute_in_subgrapgh(self, observations_id, "name", "observations", scope_id); // only search within the scope of this agent (root node)
}

bool get_status_group_id(component_t *self, char** status_id) {
	assert(self);

	/* Get root ID to restrict search to subgraph of local SWM */
	char* scope_id = 0;
	if (!get_root_node_id(self, &scope_id)) {
		printf("[%s] [ERROR] Cannot get root  Id \n", self->name);
		return false;
	}

	return get_node_by_attribute_in_subgrapgh(self, status_id, "name", "status", scope_id); // only search within the scope of this agent (root node)
}

bool get_node_by_attribute(component_t *self, char** node_id, const char* key, const char* value) {
	assert(self);
	*node_id = NULL;
	char *msg;
	char *reply;

	// e.g.
	//    {
	//      "@worldmodeltype": "RSGQuery",
	//      "query": "GET_NODES",
	//      "attributes": [
	//          {"key": "name", "value": "observations"},
	//      ]
	//    }

	json_t *getNodeMsg = json_object();
	json_object_set_new(getNodeMsg, "@worldmodeltype", json_string("RSGQuery"));
	json_object_set_new(getNodeMsg, "query", json_string("GET_NODES"));
	json_t* nodeAttribute = json_object();
	json_object_set_new(nodeAttribute, "key", json_string(key));
	json_object_set_new(nodeAttribute, "value", json_string(value));
	json_t* originAttributes = json_array();
	json_array_append_new(originAttributes, nodeAttribute);
	json_object_set_new(getNodeMsg, "attributes", originAttributes);

	/* Send message and wait for reply */
	msg = encode_json_message(self, getNodeMsg);
	shout_message(self, msg);
	reply = wait_for_reply(self, msg, self->timeout);
	printf("#########################################\n");
	printf("[%s] Got reply for get_node_by_attribute: %s \n", self->name, reply);

	/* Parse reply */
    json_error_t error;
	json_t* nodeIdReply = json_loads(reply, 0, &error);
	free(msg);
	free(reply);
	json_t* nodeIdAsJSON = 0;
	json_t* array = json_object_get(nodeIdReply, "ids");
	if (array) {
		printf("[%s] result array found: \n", self->name);
		if( json_array_size(array) > 0 ) {
			nodeIdAsJSON = json_array_get(array, 0);
			*node_id = strdup(json_string_value(nodeIdAsJSON));
			printf("[%s] get_node_by_attribute ID is: %s \n", self->name, *node_id);
		} else {
			json_decref(nodeIdReply);
			json_decref(getNodeMsg);
			return false;
		}
	} else {
		json_decref(nodeIdReply);
		json_decref(getNodeMsg);
		return false;
	}

	/* Clean up */
	json_decref(nodeIdReply);
	json_decref(getNodeMsg);

	return true;
}

bool get_node_by_attribute_in_subgrapgh(component_t *self, char** node_id, const char* key, const char* value,  const char* subgraph_id) {
	assert(self);
	*node_id = NULL;
	char *msg;
	char *reply;

	// e.g.
	//    {
	//      "@worldmodeltype": "RSGQuery",
	//      "query": "GET_NODES",
	//      "subgraphId": "b0890bef-59fa-42f5-b195-cf5e28240d7d",
	//      "attributes": [
	//          {"key": "name", "value": "observations"},
	//      ]
	//    }

	json_t *getNodeMsg = json_object();
	json_object_set_new(getNodeMsg, "@worldmodeltype", json_string("RSGQuery"));
	json_object_set_new(getNodeMsg, "query", json_string("GET_NODES"));
	json_object_set_new(getNodeMsg, "subgraphId", json_string(subgraph_id));
	json_t* nodeAttribute = json_object();
	json_object_set_new(nodeAttribute, "key", json_string(key));
	json_object_set_new(nodeAttribute, "value", json_string(value));
	json_t* originAttributes = json_array();
	json_array_append_new(originAttributes, nodeAttribute);
	json_object_set_new(getNodeMsg, "attributes", originAttributes);

	/* Send message and wait for reply */
	msg = encode_json_message(self, getNodeMsg);
	shout_message(self, msg);
	reply = wait_for_reply(self, msg, self->timeout);
	printf("#########################################\n");
	printf("[%s] Got reply for get_node_by_attribute: %s \n", self->name, reply);

	/* Parse reply */
    json_error_t error;
	json_t* nodeIdReply = json_loads(reply, 0, &error);
	free(msg);
	free(reply);
	json_t* nodeIdAsJSON = 0;
	json_t* array = json_object_get(nodeIdReply, "ids");
	if (array) {
		printf("[%s] result array found: \n", self->name);
		if( json_array_size(array) > 0 ) {
			nodeIdAsJSON = json_array_get(array, 0);
			*node_id = strdup(json_string_value(nodeIdAsJSON));
//			printf("[%s] get_node_by_attribute ID is: %s \n", self->name, *node_id);
		} else {
			json_decref(nodeIdReply);
			json_decref(getNodeMsg);
			return false;
		}
	} else {
		json_decref(nodeIdReply);
		json_decref(getNodeMsg);
		return false;
	}

	/* Clean up */
	json_decref(nodeIdReply);
	json_decref(getNodeMsg);

	return true;
}

bool add_geopose_to_node(component_t *self, char* node_id, char** new_geopose_id, double* transform_matrix, double utc_time_stamp_in_mili_sec, const char* key, const char* value, int max_cache_duration_in_sec) {
	assert(self);
	*new_geopose_id = NULL;
	char *msg;
	char *reply;

	char* originId = 0;
    if(!get_gis_origin_id(self, &originId)) {
    	printf("[%s] [ERROR] Cannot get origin Id \n", self->name);
    	return false;
    }
	printf("[%s] add_geopose_to_node origin Id = %s \n", self->name, originId);

	zuuid_t *uuid = zuuid_new ();
	assert(uuid);
	*new_geopose_id = zuuid_str_canonical(uuid);

	/*
	 * Pose
	 */
	//    newTransformMsg = {
	//      "@worldmodeltype": "RSGUpdate",
	//      "operation": "CREATE",
	//      "node": {
	//        "@graphtype": "Connection",
	//        "@semanticContext":"Transform",
	//        "id": tfId,
	//        "attributes": [
	//          {"key": "tf:type", "value": "wgs84"}
	//        ],
	//        "sourceIds": [
	//          originId,
	//        ],
	//        "targetIds": [
	//          imageNodeId,
	//        ],
	//        "history" : [
	//          {
	//            "stamp": {
	//              "@stamptype": "TimeStampDate",
	//              "stamp": currentTimeStamp,
	//            },
	//            "transform": {
	//              "type": "HomogeneousMatrix44",
	//                "matrix": [
	//                  [1,0,0,x],
	//                  [0,1,0,y],
	//                  [0,0,1,z],
	//                  [0,0,0,1]
	//                ],
	//                "unit": "latlon"
	//            }
	//          }
	//        ],
	//      },
	//      "parentId": originId,
	//    }

	// top level message
	json_t *newTfNodeMsg = json_object();
	json_object_set_new(newTfNodeMsg, "@worldmodeltype", json_string("RSGUpdate"));
	json_object_set_new(newTfNodeMsg, "operation", json_string("CREATE"));
	json_object_set_new(newTfNodeMsg, "parentId", json_string(originId));
	json_t *newTfConnection = json_object();
	json_object_set_new(newTfConnection, "@graphtype", json_string("Connection"));
	json_object_set_new(newTfConnection, "@semanticContext", json_string("Transform"));
	zuuid_t *poseUuid = zuuid_new ();
	json_object_set_new(newTfConnection, "id", json_string(*new_geopose_id));
	// Attributes
	json_t *poseAttribute = json_object();
	json_object_set_new(poseAttribute, "key", json_string("tf:type"));
	json_object_set_new(poseAttribute, "value", json_string("wgs84"));
	json_t *poseAttributes = json_array();
	json_array_append_new(poseAttributes, poseAttribute);
	if((key != NULL) && (value != NULL)) { // Optionally append a second generic attribute
		json_t *poseAttribute2 = json_object();
		json_object_set_new(poseAttribute2, "key", json_string(key));
		json_object_set_new(poseAttribute2, "value", json_string(value));
		json_array_append_new(poseAttributes, poseAttribute2);
	}

	const int default_cache_duration_in_sec = 10;
	if (max_cache_duration_in_sec != default_cache_duration_in_sec) {
		json_t *poseAttribute3 = json_object();
		json_object_set_new(poseAttribute3, "key", json_string("tf:max_duration"));
		char duration[512] = {0};
		snprintf(duration, sizeof(duration), "%i%s", max_cache_duration_in_sec, "s");
		json_object_set_new(poseAttribute3, "value", json_string(duration));
		json_array_append_new(poseAttributes, poseAttribute3);
	}

	json_object_set_new(newTfConnection, "attributes", poseAttributes);
	// sourceIds
	json_t *sourceIds = json_array();
	json_array_append_new(sourceIds, json_string(originId)); // ID of origin node
	json_object_set_new(newTfConnection, "sourceIds", sourceIds);
	// sourceIds
	json_t *targetIds = json_array();
	json_array_append_new(targetIds, json_string(node_id)); // ID of node that we just have created before
	json_object_set_new(newTfConnection, "targetIds", targetIds);

	// history
	json_t *history = json_array();
	json_t *stampedPose = json_object();
	json_t *stamp = json_object();
	json_t *pose = json_object();

	// stamp
	json_object_set_new(stamp, "@stamptype", json_string("TimeStampUTCms"));
	json_object_set_new(stamp, "stamp", json_real(utc_time_stamp_in_mili_sec));

	/* column-major layout:
	 * 0 4 8  12
	 * 1 5 9  13
	 * 2 6 10 14
	 * 3 7 11 15
	 *
	 *  <=>
	 *
	 * r11 r12 r13  x
	 * r21 r22 r23  y
	 * r31 r32 r33  z
	 * 3    7   11  15
	 */

	//pose
	json_object_set_new(pose, "type", json_string("HomogeneousMatrix44"));
	json_object_set_new(pose, "unit", json_string("latlon"));
	json_t *matrix = json_array();
	json_t *row0 = json_array();
	json_array_append_new(row0, json_real(transform_matrix[0]));
	json_array_append_new(row0, json_real(transform_matrix[4]));
	json_array_append_new(row0, json_real(transform_matrix[8]));
	json_array_append_new(row0, json_real(transform_matrix[12]));
	json_t *row1 = json_array();
	json_array_append_new(row1, json_real(transform_matrix[1]));
	json_array_append_new(row1, json_real(transform_matrix[5]));
	json_array_append_new(row1, json_real(transform_matrix[9]));
	json_array_append_new(row1, json_real(transform_matrix[13]));
	json_t *row2 = json_array();
	json_array_append_new(row2, json_real(transform_matrix[2]));
	json_array_append_new(row2, json_real(transform_matrix[6]));
	json_array_append_new(row2, json_real(transform_matrix[10]));
	json_array_append_new(row2, json_real(transform_matrix[14]));
	json_t *row3 = json_array();
	json_array_append_new(row3, json_real(transform_matrix[3]));
	json_array_append_new(row3, json_real(transform_matrix[7]));
	json_array_append_new(row3, json_real(transform_matrix[11]));
	json_array_append_new(row3, json_real(transform_matrix[15]));
	json_array_append_new(matrix, row0);
	json_array_append_new(matrix, row1);
	json_array_append_new(matrix, row2);
	json_array_append_new(matrix, row3);

	json_object_set_new(pose, "matrix", matrix);

	json_object_set_new(stampedPose, "stamp", stamp);
	json_object_set_new(stampedPose, "transform", pose);
	json_array_append_new(history, stampedPose);
	json_object_set_new(newTfConnection, "history", history);
	json_object_set_new(newTfNodeMsg, "node", newTfConnection);

	/* Send message and wait for reply */
	msg = encode_json_message(self, newTfNodeMsg);
	shout_message(self, msg);
	reply = wait_for_reply(self, msg, self->timeout);
	printf("#########################################\n");
	printf("[%s] Got reply for pose: %s \n", self->name, reply);

	/* Parse reply */
	json_error_t error;
	json_t* tfReply = json_loads(reply, 0, &error);
	json_t* querySuccessMsg = json_object_get(tfReply, "updateSuccess");
	bool querySuccess = false;
	char* dump = json_dumps(querySuccessMsg, JSON_ENCODE_ANY);
	printf("[%s] querySuccessMsg is: %s \n", self->name, dump);
	free(dump);
	if (querySuccessMsg) {
		querySuccess = json_is_true(querySuccessMsg);
	}

	/* Clean up */
	free(msg);
	free(reply);
	free(uuid);
	free(poseUuid);
	free(originId);
	json_decref(newTfNodeMsg);
	json_decref(tfReply);

	return querySuccess;
}

bool get_mediator_id(component_t *self, char** mediator_id) {
	assert(self);
	char* reply = NULL;

	// Generate message
	//    {
	//      "metamodel": "sherpa_mgs",
	//      "model": "http://kul/query_mediator_uuid.json",
	//      "type": "query_mediator_uuid",
	//		"payload":
	//		{
	//			"UID": 2147aba0-0d59-41ec-8531-f6787fe52b60
	//		}
	//    }
	json_t *getMediatorIDMsg = json_object();
	json_object_set_new(getMediatorIDMsg, "metamodel", json_string("sherpa_mgs"));
	json_object_set_new(getMediatorIDMsg, "model", json_string("http://kul/query_mediator_uuid.json"));
	json_object_set_new(getMediatorIDMsg, "type", json_string("query_mediator_uuid"));
	json_t* pl = json_object();
	zuuid_t *uuid = zuuid_new ();
	assert(uuid);
	json_object_set_new(pl, "UID", json_string(zuuid_str_canonical(uuid)));
	json_object_set_new(getMediatorIDMsg, "payload", pl);

	// add it to the query list
	json_msg_t *msg = (json_msg_t *) zmalloc (sizeof (json_msg_t));
	msg->metamodel = strdup("sherpa_mgs");
	msg->model = strdup("http://kul/query_mediator_uuid.json");
	msg->type = strdup("query_mediator_uuid");
	msg->payload = json_dumps(pl, JSON_ENCODE_ANY);
	query_t * q = query_new(zuuid_str_canonical(uuid), zyre_uuid(self->local), msg, NULL);
	zlist_append(self->query_list, q);
	free(uuid);

	char* ret = json_dumps(getMediatorIDMsg, JSON_ENCODE_ANY);
	printf("[%s] send_json_message: message = %s:\n", self->name, ret);
	json_decref(getMediatorIDMsg);

	/* Send message and wait for reply */
	shout_message(self, ret);
	reply = wait_for_reply(self, ret, self->timeout);
	if (reply==0) {
		printf("[%s] Received no reply for mediator_id query.\n", self->name);
		free(msg);
		free(ret);
		free(reply);
		return false;
	}
	//printf("[%s] Got reply for query_mediator_uuid: %s \n", self->name, reply);

	/* Parse reply */
    json_error_t error;
	json_t* rep = json_loads(reply, 0, &error);
//	free(msg); wait_for_reply handles this
	free(ret);
	free(reply);
	if(!rep) {
		printf("Error parsing JSON string! line %d: %s\n", error.line, error.text);
		return false;
	}
	*mediator_id = strdup(json_string_value(json_object_get(rep, "remote")));
	json_decref(rep);
	if (!*mediator_id) {
		printf("Reply did not contain mediator ID.\n");
		return false;
	}

	return true;
}

bool add_victim(component_t *self, double* transform_matrix, double utc_time_stamp_in_mili_sec, char* author) {

	if (self == NULL) {
		return false;
		printf("[ERROR] Communication component is not yet initialized.\n");
	}

	char* msg;
	char* reply;
    json_error_t error;

	/* Get observationGroupId */
	char* observationGroupId = 0;
    if(!get_observations_group_id(self, &observationGroupId)) {
    	printf("[%s] [ERROR] Cannot get observation group Id \n", self->name);
    	return false;
    }
	printf("[%s] observation Id = %s \n", self->name, observationGroupId);

    /*
     * Get the "origin" node. It is relevant to specify a new pose.
     */
	char* originId = 0;
    if(!get_gis_origin_id(self, &originId)) {
    	printf("[%s] [ERROR] Cannot get origin Id \n", self->name);
    	return false;
    }
	printf("[%s] origin Id = %s \n", self->name, originId);


	/*
	 * The actual "observation" node. Here for a victim.
	 */

	//    currentTimeStamp = datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S')
	//    imageNodeId = str(uuid.uuid4())
	//    newImageNodeMsg = {
	//      "@worldmodeltype": "RSGUpdate",
	//      "operation": "CREATE",
	//      "node": {
	//        "@graphtype": "Node",
	//        "id": imageNodeId,
	//        "attributes": [
	//              {"key": "sherpa:observation_type", "value": "image"},
	//              {"key": "sherpa:uri", "value": URI},
	//              {"key": "sherpa:stamp", "value": currentTimeStamp},
	//              {"key": "sherpa:author", "value": author},
	//        ],
	//      },
	//      "parentId": observationGroupId,
	//    }

	// top level message
	json_t *newImageNodeMsg = json_object();
	json_object_set_new(newImageNodeMsg, "@worldmodeltype", json_string("RSGUpdate"));
	json_object_set_new(newImageNodeMsg, "operation", json_string("CREATE"));
	json_object_set_new(newImageNodeMsg, "parentId", json_string(observationGroupId));
	json_t *newImageNode = json_object();
	json_object_set_new(newImageNode, "@graphtype", json_string("Node"));
	zuuid_t *uuid = zuuid_new ();
	json_object_set_new(newImageNode, "id", json_string(zuuid_str_canonical(uuid)));

	// attributes
	json_t *newObservationAttributes = json_array();
	json_t *attribute1 = json_object();
	json_object_set_new(attribute1, "key", json_string("sherpa:observation_type"));
	json_object_set_new(attribute1, "value", json_string("victim"));
	json_array_append_new(newObservationAttributes, attribute1);
	//	json_t *attribute2 = json_object();
	//	json_object_set(attribute2, "key", json_string("sherpa:uri"));
	//	json_object_set(attribute2, "value", json_string("TODO"));
	//	json_array_append(attributes, attribute2);
	json_t *attribute3 = json_object();
	json_object_set_new(attribute3, "key", json_string("sherpa:stamp"));
	json_object_set_new(attribute3, "value", json_real(utc_time_stamp_in_mili_sec));
	json_array_append_new(newObservationAttributes, attribute3);
	json_t *attribute4 = json_object();
	json_object_set_new(attribute4, "key", json_string("sherpa:author"));
	json_object_set_new(attribute4, "value", json_string(author));
	json_array_append_new(newObservationAttributes, attribute4);


	json_object_set_new(newImageNode, "attributes", newObservationAttributes);
	json_object_set_new(newImageNodeMsg, "node", newImageNode);

	/* CReate message*/
	msg = encode_json_message(self, newImageNodeMsg);
	/* Send the message */
	shout_message(self, msg);
	/* Wait for a reply */
	reply = wait_for_reply(self, msg, self->timeout);
	/* Print reply */
	printf("#########################################\n");
	printf("[%s] Got reply: %s \n", self->name, reply);


	char* poseId;
	bool succsess = add_geopose_to_node(self, zuuid_str_canonical(uuid), &poseId, transform_matrix, utc_time_stamp_in_mili_sec, 0, 0, 10);


	/* Clean up */
	free(msg);
	free(reply);
	free(observationGroupId);
	free(originId);
	free(uuid);
	json_decref(newImageNodeMsg);

	return succsess;
}

bool add_image(component_t *self, double* transform_matrix, double utc_time_stamp_in_mili_sec, char* author, char* file_name) {
	if (self == NULL) {
		return false;
		printf("[ERROR] Communication component is not yet initialized.\n");
	}

	char* msg;
	char* reply;
    json_error_t error;

	/* Get observationGroupId */
	char* observationGroupId = 0;
    if(!get_observations_group_id(self, &observationGroupId)) {
    	printf("[%s] [ERROR] Cannot get observation group Id \n", self->name);
    	return false;
    }
	printf("[%s] observation Id = %s \n", self->name, observationGroupId);

    /*
     * Get the "origin" node. It is relevant to specify a new pose.
     */
	char* originId = 0;
    if(!get_gis_origin_id(self, &originId)) {
    	printf("[%s] [ERROR] Cannot get origin Id \n", self->name);
    	return false;
    }
	printf("[%s] origin Id = %s \n", self->name, originId);

	/* get Mediator ID */
	char* mediator_uuid; //= "79346b2b-e0a1-4e04-a7c8-981828436357";
	if(!get_mediator_id(self, &mediator_uuid)) {
		printf("[%s] [ERROR] Cannot get Mediator ID. Is the Mediator started? \n", self->name);
		return false;
	}

    char uri[1024] = {0};
    snprintf(uri, sizeof(uri), "%s:%s", mediator_uuid, file_name);

	/*
	 * The actual "observation" node. Here for an image.
	 */

	//    currentTimeStamp = datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S')
	//    imageNodeId = str(uuid.uuid4())
	//    newImageNodeMsg = {
	//      "@worldmodeltype": "RSGUpdate",
	//      "operation": "CREATE",
	//      "node": {
	//        "@graphtype": "Node",
	//        "id": imageNodeId,
	//        "attributes": [
	//              {"key": "sherpa:observation_type", "value": "image"},
	//              {"key": "sherpa:uri", "value": URI},
	//              {"key": "sherpa:stamp", "value": currentTimeStamp},
	//              {"key": "sherpa:author", "value": author},
	//        ],
	//      },
	//      "parentId": observationGroupId,
	//    }

	// top level message
	json_t *newImageNodeMsg = json_object();
	json_object_set_new(newImageNodeMsg, "@worldmodeltype", json_string("RSGUpdate"));
	json_object_set_new(newImageNodeMsg, "operation", json_string("CREATE"));
	json_object_set_new(newImageNodeMsg, "parentId", json_string(observationGroupId));
	json_t *newImageNode = json_object();
	json_object_set_new(newImageNode, "@graphtype", json_string("Node"));
	zuuid_t *uuid = zuuid_new ();
	json_object_set_new(newImageNode, "id", json_string(zuuid_str_canonical(uuid)));

	// attributes
	json_t *newObservationAttributes = json_array();
	json_t *attribute1 = json_object();
	json_object_set_new(attribute1, "key", json_string("sherpa:observation_type"));
	json_object_set_new(attribute1, "value", json_string("image"));
	json_array_append_new(newObservationAttributes, attribute1);
	json_t *attribute2 = json_object();
	json_object_set_new(attribute2, "key", json_string("sherpa:uri"));
	json_object_set_new(attribute2, "value", json_string(uri));
	json_array_append_new(newObservationAttributes, attribute2);
	json_t *attribute3 = json_object();
	json_object_set_new(attribute3, "key", json_string("sherpa:stamp"));
	json_object_set_new(attribute3, "value", json_real(utc_time_stamp_in_mili_sec));
	json_array_append_new(newObservationAttributes, attribute3);
	json_t *attribute4 = json_object();
	json_object_set_new(attribute4, "key", json_string("sherpa:author"));
	json_object_set_new(attribute4, "value", json_string(author));
	json_array_append_new(newObservationAttributes, attribute4);


	json_object_set_new(newImageNode, "attributes", newObservationAttributes);
	json_object_set_new(newImageNodeMsg, "node", newImageNode);

	/* CReate message*/
	msg = encode_json_message(self, newImageNodeMsg);
	/* Send the message */
	shout_message(self, msg);
	/* Wait for a reply */
	reply = wait_for_reply(self, msg, self->timeout);
	/* Print reply */
	printf("#########################################\n");
	printf("[%s] Got reply: %s \n", self->name, reply);


	char* poseId;
	bool succsess = add_geopose_to_node(self, zuuid_str_canonical(uuid), &poseId, transform_matrix, utc_time_stamp_in_mili_sec, 0, 0 , 10);


	/* Clean up */
	free(msg);
	free(reply);
	free(observationGroupId);
	free(originId);
	free(uuid);
	free(mediator_uuid);
	json_decref(newImageNodeMsg);

	return succsess;
}

#if 0
bool add_artva(component_t *self, double* transform_matrix, double artva0, double artva1, double artva2, double artva3,
		double utc_time_stamp_in_mili_sec, char* author) {

	if (self == NULL) {
		return false;
		printf("[ERROR] Communication component is not yet initialized.\n");
	}

	char* msg;
	char* reply;
    json_error_t error;

	/* Get observationGroupId */
	char* observationGroupId = 0;
    if(!get_observations_group_id(self, &observationGroupId)) {
    	printf("[%s] [ERROR] Cannot get observation group Id \n", self->name);
    	return false;
    }
	printf("[%s] observation Id = %s \n", self->name, observationGroupId);

    /*
     * Get the "origin" node. It is relevant to specify a new pose.
     */
	char* originId = 0;
    if(!get_gis_origin_id(self, &originId)) {
    	printf("[%s] [ERROR] Cannot get origin Id \n", self->name);
    	return false;
    }
	printf("[%s] origin Id = %s \n", self->name, originId);

	/*
	 * The actual "observation" node. Here for an an artva signal.
	 */

	//    currentTimeStamp = datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S')
	//    imageNodeId = str(uuid.uuid4())
	//    newImageNodeMsg = {
	//      "@worldmodeltype": "RSGUpdate",
	//      "operation": "CREATE",
	//      "node": {
	//        "@graphtype": "Node",
	//        "id": imageNodeId,
	//        "attributes": [
	//              {"key": "sherpa:observation_type", "value": "artva"},
    //              {"key": "sherpa:artva_signal0", "value": "77"},
    //              {"key": "sherpa:artva_signal1", "value": "12"},
    //              {"key": "sherpa:artva_signal2", "value": "0"},
    //              {"key": "sherpa:artva_signal3", "value": "0"},
	// OPTIONAL:
	//              {"key": "sherpa:stamp", "value": currentTimeStamp},
	//              {"key": "sherpa:author", "value": author},
	//        ],
	//      },
	//      "parentId": observationGroupId,
	//    }

	// top level message
	json_t *newARTVANodeMsg = json_object();
	json_object_set_new(newARTVANodeMsg, "@worldmodeltype", json_string("RSGUpdate"));
	json_object_set_new(newARTVANodeMsg, "operation", json_string("CREATE"));
	json_object_set_new(newARTVANodeMsg, "parentId", json_string(observationGroupId));
	json_t *newImageNode = json_object();
	json_object_set_new(newImageNode, "@graphtype", json_string("Node"));
	zuuid_t *uuid = zuuid_new ();
	json_object_set_new(newImageNode, "id", json_string(zuuid_str_canonical(uuid)));

	// attributes
	json_t *newObservationAttributes = json_array();
	json_t *attribute1 = json_object();
	json_object_set_new(attribute1, "key", json_string("sherpa:observation_type"));
	json_object_set_new(attribute1, "value", json_string("artva"));
	json_array_append_new(newObservationAttributes, attribute1);
	json_t *attribute2a = json_object();
	json_object_set_new(attribute2a, "key", json_string("sherpa:artva_signal0"));
	json_object_set_new(attribute2a, "value", json_real(artva0));
	json_array_append_new(newObservationAttributes, attribute2a);
	json_t *attribute2b = json_object();
	json_object_set_new(attribute2b, "key", json_string("sherpa:artva_signal1"));
	json_object_set_new(attribute2b, "value", json_real(artva1));
	json_array_append_new(newObservationAttributes, attribute2b);
	json_t *attribute2c = json_object();
	json_object_set_new(attribute2c, "key", json_string("sherpa:artva_signal2"));
	json_object_set_new(attribute2c, "value", json_real(artva2));
	json_array_append_new(newObservationAttributes, attribute2c);
	json_t *attribute2d = json_object();
	json_object_set_new(attribute2d, "key", json_string("sherpa:artva_signal3"));
	json_object_set_new(attribute2d, "value", json_real(artva3));
	json_array_append_new(newObservationAttributes, attribute2d);
	json_t *attribute3 = json_object();
	json_object_set_new(attribute3, "key", json_string("sherpa:stamp"));
	json_object_set_new(attribute3, "value", json_real(utc_time_stamp_in_mili_sec));
	json_array_append_new(newObservationAttributes, attribute3);
	json_t *attribute4 = json_object();
	json_object_set_new(attribute4, "key", json_string("sherpa:author"));
	json_object_set_new(attribute4, "value", json_string(author));
	json_array_append_new(newObservationAttributes, attribute4);


	json_object_set_new(newImageNode, "attributes", newObservationAttributes);
	json_object_set_new(newARTVANodeMsg, "node", newImageNode);

	/* CReate message*/
	msg = encode_json_message(self, newARTVANodeMsg);
	/* Send the message */
	shout_message(self, msg);
	/* Wait for a reply */
	reply = wait_for_reply(self, msg, self->timeout);
	/* Print reply */
	printf("#########################################\n");
	printf("[%s] Got reply: %s \n", self->name, reply);


	const char* poseId;
	bool succsess = add_geopose_to_node(self, zuuid_str_canonical(uuid), &poseId, transform_matrix, utc_time_stamp_in_mili_sec, 0, 0);


	/* Clean up */
	free(msg);
	free(reply);
	free(observationGroupId);
	free(originId);
	free(uuid);
	json_decref(newARTVANodeMsg);

	return succsess;
}
#endif

bool add_artva_measurement(component_t *self, artva_measurement measurement, char* author) {
	if (self == NULL) {
		return false;
		printf("[ERROR] Communication component is not yet initialized.\n");
	}

	char* msg;
	char* reply;
    json_error_t error;


	/* Get root ID to restrict search to subgraph of local SWM */
	char* scope_id = 0;
	if (!get_node_by_attribute(self, &scope_id, "sherpa:agent_name", author)) { // only search within the scope of this agent
		printf("[%s] [ERROR] Cannot get cope Id \n", self->name);
		return false;
	}

	/* prepare payload */
	// attributes
	json_t* attributes = json_array();
	json_t* attribute1 = json_object();
	json_object_set_new(attribute1, "key", json_string("sherpa:observation_type"));
	json_object_set_new(attribute1, "value", json_string("artva"));
	json_array_append_new(attributes, attribute1);

	json_t* attribute2 = json_object();
	json_object_set_new(attribute2, "key", json_string("sherpa:artva_signal0"));
	json_object_set_new(attribute2, "value", json_integer(measurement.signal0));
	json_array_append_new(attributes, attribute2);

	json_t* attribute3 = json_object();
	json_object_set_new(attribute3, "key", json_string("sherpa:artva_signal1"));
	json_object_set_new(attribute3, "value", json_integer(measurement.signal1));
	json_array_append_new(attributes, attribute3);

	json_t* attribute4 = json_object();
	json_object_set_new(attribute4, "key", json_string("sherpa:artva_signal2"));
	json_object_set_new(attribute4, "value", json_integer(measurement.signal2));
	json_array_append_new(attributes, attribute4);

	json_t* attribute5 = json_object();
	json_object_set_new(attribute5, "key", json_string("sherpa:artva_signal3"));
	json_object_set_new(attribute5, "value", json_integer(measurement.signal3));
	json_array_append_new(attributes, attribute5);

	json_t* attribute6 = json_object();
	json_object_set_new(attribute6, "key", json_string("sherpa:artva_angle0"));
	json_object_set_new(attribute6, "value", json_integer(measurement.angle0));
	json_array_append_new(attributes, attribute6);

	json_t* attribute7 = json_object();
	json_object_set_new(attribute7, "key", json_string("sherpa:artva_angle1"));
	json_object_set_new(attribute7, "value", json_integer(measurement.angle1));
	json_array_append_new(attributes, attribute7);

	json_t* attribute8 = json_object();
	json_object_set_new(attribute8, "key", json_string("sherpa:artva_angle2"));
	json_object_set_new(attribute8, "value", json_integer(measurement.angle2));
	json_array_append_new(attributes, attribute8);

	json_t* attribute9 = json_object();
	json_object_set_new(attribute9, "key", json_string("sherpa:artva_angle3"));
	json_object_set_new(attribute9, "value", json_integer(measurement.angle3));
	json_array_append_new(attributes, attribute9);

	char* artvaId = 0;
	if (!get_node_by_attribute_in_subgrapgh(self, &artvaId, "sherpa:observation_type", "artva", scope_id)) { // measurement node does not exist yet, so we will add it here

		/* Get observationGroupId */
		char* observationGroupId = 0;
		if(!get_observations_group_id(self, &observationGroupId)) {
			printf("[%s] [ERROR] Cannot get observation group Id \n", self->name);
			return false;
		}
		printf("[%s] observation Id = %s \n", self->name, observationGroupId);


		json_t *newArtvaMeasurementMsg = json_object();
		json_object_set_new(newArtvaMeasurementMsg, "@worldmodeltype", json_string("RSGUpdate"));
		json_object_set_new(newArtvaMeasurementMsg, "operation", json_string("CREATE"));
		json_object_set_new(newArtvaMeasurementMsg, "parentId", json_string(observationGroupId));
		json_t *newArtvaNode = json_object();
		json_object_set_new(newArtvaNode, "@graphtype", json_string("Node"));
		zuuid_t *uuid = zuuid_new ();
		char* artvaNodeId = zuuid_str_canonical(uuid);
		json_object_set_new(newArtvaNode, "id", json_string(artvaNodeId));

		// attributes see above

		json_object_set_new(newArtvaNode, "attributes", attributes);
		json_object_set_new(newArtvaMeasurementMsg, "node", newArtvaNode);

		/* CReate message*/
		msg = encode_json_message(self, newArtvaMeasurementMsg);
		/* Send the message */
		shout_message(self, msg);
		/* Wait for a reply */
		reply = wait_for_reply(self, msg, self->timeout);
		/* Print reply */
		printf("#########################################\n");
		printf("[%s] Got reply: %s \n", self->name, reply);

		/* Parse reply */
		json_t* newArtvaNodeReply = json_loads(reply, 0, &error);
		json_t* querySuccessMsg = json_object_get(newArtvaNodeReply, "updateSuccess");
		bool querySuccess = false;
		char* dump = json_dumps(querySuccessMsg, JSON_ENCODE_ANY);
		printf("[%s] querySuccessMsg is: %s \n", self->name, dump);
		free(dump);

		if (querySuccessMsg) {
			querySuccess = json_is_true(querySuccessMsg);
		}

		json_decref(newArtvaMeasurementMsg);
		json_decref(newArtvaNodeReply);
		free(msg);
		free(reply);
		free(artvaNodeId);
		free(uuid);

		if(!querySuccess) {
			printf("[%s] [ERROR] Can not add battery node for agent.\n", self->name);
			return false;
		}

		return true;
	} // it exists, so just update it

	// if it exists already, just UPDATE the attributes

		//        batteryUpdateMsg = {
		//          "@worldmodeltype": "RSGUpdate",
		//          "operation": "UPDATE_ATTRIBUTES",
		//          "node": {
		//            "@graphtype": "Node",
		//            "id": self.battery_uuid,
		//            "attributes": [
		//                  {"key": "sensor:battery_voltage", "value": self.battery_voltage},
		//            ],
		//           },
		//        }

	json_t *updateArtvaNodeMsg = json_object();
	json_object_set_new(updateArtvaNodeMsg, "@worldmodeltype", json_string("RSGUpdate"));
	json_object_set_new(updateArtvaNodeMsg, "operation", json_string("UPDATE_ATTRIBUTES"));
	json_t *updateArtvaNode = json_object();
	json_object_set_new(updateArtvaNode, "@graphtype", json_string("Node"));
	json_object_set_new(updateArtvaNode, "id", json_string(artvaId));

	// attributes see above

	json_object_set_new(updateArtvaNode, "attributes", attributes);
	json_object_set_new(updateArtvaNodeMsg, "node", updateArtvaNode);

	/* CReate message*/
	msg = encode_json_message(self, updateArtvaNodeMsg);
	/* Send the message */
	shout_message(self, msg);
	/* Wait for a reply */
	reply = wait_for_reply(self, msg, self->timeout);
	/* Print reply */
	printf("#########################################\n");
	printf("[%s] Got reply: %s \n", self->name, reply);

	/* Parse reply */
	json_t* updateArtvaReply = json_loads(reply, 0, &error);
	json_t* querySuccessMsg = json_object_get(updateArtvaReply, "updateSuccess");
	bool updateSuccess = false;
	char* dump = json_dumps(querySuccessMsg, JSON_ENCODE_ANY);
	printf("[%s] querySuccessMsg is: %s \n", self->name, dump);
	free(dump);

	if (querySuccessMsg) {
		//updateSuccess = json_is_true(querySuccessMsg);
		updateSuccess = true; //FIXME updates of same values are not yet supported
	}

	json_decref(updateArtvaNodeMsg);
	json_decref(updateArtvaReply);
	free(msg);
	free(reply);

	return updateSuccess;

}

bool add_wasp_flight_status(component_t *self, wasp_flight_status status, char* author) {
	if (self == NULL) {
		return false;
		printf("[ERROR] Communication component is not yet initialized.\n");
	}

	char* msg;
	char* reply;
    json_error_t error;


	/* Get root ID to restrict search to subgraph of local SWM */
	char* scope_id = 0;
	if (!get_node_by_attribute(self, &scope_id, "sherpa:agent_name", author)) { // only search within the scope of this agent
		printf("[%s] [ERROR] Cannot get cope Id \n", self->name);
		return false;
	}

	/* prepare payload */
	// attributes
	json_t* attributes = json_array();
	json_t* attribute1 = json_object();
	json_object_set_new(attribute1, "key", json_string("sherpa:status_type"));
	json_object_set_new(attribute1, "value", json_string("wasp"));
	json_array_append_new(attributes, attribute1);

	json_t* attribute2 = json_object();
	json_object_set_new(attribute2, "key", json_string("sherpa:wasp_flight_state"));
	json_object_set_new(attribute2, "value", json_string(status.flight_state));
	json_array_append_new(attributes, attribute2);

	char* waspStatusId = 0;
	if (!get_node_by_attribute_in_subgrapgh(self, &waspStatusId, "sherpa:status_type", "wasp", scope_id)) { // measurement node does not exist yet, so we will add it here

		/* Get observationGroupId */
		char* statusGroupId = 0;
		if(!get_status_group_id(self, &statusGroupId)) {
			printf("[%s] [ERROR] Cannot get status group Id \n", self->name);
			return false;
		}
		printf("[%s] status Id = %s \n", self->name, statusGroupId);


		json_t *newWaspStatusMsg = json_object();
		json_object_set_new(newWaspStatusMsg, "@worldmodeltype", json_string("RSGUpdate"));
		json_object_set_new(newWaspStatusMsg, "operation", json_string("CREATE"));
		json_object_set_new(newWaspStatusMsg, "parentId", json_string(statusGroupId));
		json_t *newWaspNode = json_object();
		json_object_set_new(newWaspNode, "@graphtype", json_string("Node"));
		zuuid_t *uuid = zuuid_new ();
		char* waspNodeId = zuuid_str_canonical(uuid);
		json_object_set_new(newWaspNode, "id", json_string(waspNodeId));

		// attributes see above

		json_object_set_new(newWaspNode, "attributes", attributes);
		json_object_set_new(newWaspStatusMsg, "node", newWaspNode);

		/* CReate message*/
		msg = encode_json_message(self, newWaspStatusMsg);
		/* Send the message */
		shout_message(self, msg);
		/* Wait for a reply */
		reply = wait_for_reply(self, msg, self->timeout);
		/* Print reply */
		printf("#########################################\n");
		printf("[%s] Got reply: %s \n", self->name, reply);

		/* Parse reply */
		json_t* newWaspStatusNodeReply = json_loads(reply, 0, &error);
		json_t* querySuccessMsg = json_object_get(newWaspStatusNodeReply, "updateSuccess");
		bool querySuccess = false;
		char* dump = json_dumps(querySuccessMsg, JSON_ENCODE_ANY);
		printf("[%s] querySuccessMsg is: %s \n", self->name, dump);
		free(dump);

		if (querySuccessMsg) {
			querySuccess = json_is_true(querySuccessMsg);
		}

		json_decref(newWaspStatusMsg);
		json_decref(newWaspStatusNodeReply);
		free(msg);
		free(reply);
		free(waspNodeId);
		free(uuid);

		if(!querySuccess) {
			printf("[%s] [ERROR] Can not add wasp status node for agent.\n", self->name);
			return false;
		}

		return true;
	} // it exists, so just update it

	// if it exists already, just UPDATE the attributes

		//        batteryUpdateMsg = {
		//          "@worldmodeltype": "RSGUpdate",
		//          "operation": "UPDATE_ATTRIBUTES",
		//          "node": {
		//            "@graphtype": "Node",
		//            "id": self.battery_uuid,
		//            "attributes": [
		//                  {"key": "sensor:battery_voltage", "value": self.battery_voltage},
		//            ],
		//           },
		//        }

	json_t *updateWaspStatusNodeMsg = json_object();
	json_object_set_new(updateWaspStatusNodeMsg, "@worldmodeltype", json_string("RSGUpdate"));
	json_object_set_new(updateWaspStatusNodeMsg, "operation", json_string("UPDATE_ATTRIBUTES"));
	json_t *updateWaspNode = json_object();
	json_object_set_new(updateWaspNode, "@graphtype", json_string("Node"));
	json_object_set_new(updateWaspNode, "id", json_string(waspStatusId));

	// attributes see above

	json_object_set_new(updateWaspNode, "attributes", attributes);
	json_object_set_new(updateWaspStatusNodeMsg, "node", updateWaspNode);

	/* CReate message*/
	msg = encode_json_message(self, updateWaspStatusNodeMsg);
	/* Send the message */
	shout_message(self, msg);
	/* Wait for a reply */
	reply = wait_for_reply(self, msg, self->timeout);
	/* Print reply */
	printf("#########################################\n");
	printf("[%s] Got reply: %s \n", self->name, reply);

	/* Parse reply */
	json_t* updateWaspStatusReply = json_loads(reply, 0, &error);
	json_t* querySuccessMsg = json_object_get(updateWaspStatusReply, "updateSuccess");
	bool updateSuccess = false;
	char* dump = json_dumps(querySuccessMsg, JSON_ENCODE_ANY);
	printf("[%s] querySuccessMsg is: %s \n", self->name, dump);
	free(dump);

	if (querySuccessMsg) {
		//updateSuccess = json_is_true(querySuccessMsg);
		updateSuccess = true; //FIXME updates of same values are not yet supported
	}

	json_decref(updateWaspStatusNodeMsg);
	json_decref(updateWaspStatusReply);
	free(msg);
	free(reply);

	return updateSuccess;
}

bool add_wasp_dock_status(component_t *self, wasp_dock_status status, char* author) {
	if (self == NULL) {
		return false;
		printf("[ERROR] Communication component is not yet initialized.\n");
	}

	char* msg;
	char* reply;
    json_error_t error;


	/* Get root ID to restrict search to subgraph of local SWM */
	char* scope_id = 0;
	if (!get_node_by_attribute(self, &scope_id, "sherpa:agent_name", author)) { // only search within the scope of this agent
		printf("[%s] [ERROR] Cannot get cope Id \n", self->name);
		return false;
	}

	/* prepare payload */
	// attributes
	json_t* attributes = json_array();
	json_t* attribute1 = json_object();
	json_object_set_new(attribute1, "key", json_string("sherpa:status_type"));
	json_object_set_new(attribute1, "value", json_string("wasp_docking"));
	json_array_append_new(attributes, attribute1);

	json_t* attribute2 = json_object();
	json_object_set_new(attribute2, "key", json_string("sherpa:wasp_on_box"));
	json_object_set_new(attribute2, "value", json_string(status.wasp_on_box));
	json_array_append_new(attributes, attribute2);

	char* waspStatusId = 0;
	if (!get_node_by_attribute_in_subgrapgh(self, &waspStatusId, "sherpa:status_type", "wasp_docking", scope_id)) { // measurement node does not exist yet, so we will add it here

		/* Get observationGroupId */
		char* statusGroupId = 0;
		if(!get_status_group_id(self, &statusGroupId)) {
			printf("[%s] [ERROR] Cannot get status group Id \n", self->name);
			return false;
		}
		printf("[%s] status Id = %s \n", self->name, statusGroupId);


		json_t *newWaspStatusMsg = json_object();
		json_object_set_new(newWaspStatusMsg, "@worldmodeltype", json_string("RSGUpdate"));
		json_object_set_new(newWaspStatusMsg, "operation", json_string("CREATE"));
		json_object_set_new(newWaspStatusMsg, "parentId", json_string(statusGroupId));
		json_t *newWaspNode = json_object();
		json_object_set_new(newWaspNode, "@graphtype", json_string("Node"));
		zuuid_t *uuid = zuuid_new ();
		char* waspNodeId = zuuid_str_canonical(uuid);
		json_object_set_new(newWaspNode, "id", json_string(waspNodeId));

		// attributes see above

		json_object_set_new(newWaspNode, "attributes", attributes);
		json_object_set_new(newWaspStatusMsg, "node", newWaspNode);

		/* CReate message*/
		msg = encode_json_message(self, newWaspStatusMsg);
		/* Send the message */
		shout_message(self, msg);
		/* Wait for a reply */
		reply = wait_for_reply(self, msg, self->timeout);
		/* Print reply */
		printf("#########################################\n");
		printf("[%s] Got reply: %s \n", self->name, reply);

		/* Parse reply */
		json_t* newWaspStatusNodeReply = json_loads(reply, 0, &error);
		json_t* querySuccessMsg = json_object_get(newWaspStatusNodeReply, "updateSuccess");
		bool querySuccess = false;
		char* dump = json_dumps(querySuccessMsg, JSON_ENCODE_ANY);
		printf("[%s] querySuccessMsg is: %s \n", self->name, dump);
		free(dump);

		if (querySuccessMsg) {
			querySuccess = json_is_true(querySuccessMsg);
		}

		json_decref(newWaspStatusMsg);
		json_decref(newWaspStatusNodeReply);
		free(msg);
		free(reply);
		free(waspNodeId);
		free(uuid);

		if(!querySuccess) {
			printf("[%s] [ERROR] Can not add wasp status node for agent.\n", self->name);
			return false;
		}

		return true;
	} // it exists, so just update it

	// if it exists already, just UPDATE the attributes

		//        batteryUpdateMsg = {
		//          "@worldmodeltype": "RSGUpdate",
		//          "operation": "UPDATE_ATTRIBUTES",
		//          "node": {
		//            "@graphtype": "Node",
		//            "id": self.battery_uuid,
		//            "attributes": [
		//                  {"key": "sensor:battery_voltage", "value": self.battery_voltage},
		//            ],
		//           },
		//        }

	json_t *updateWaspStatusNodeMsg = json_object();
	json_object_set_new(updateWaspStatusNodeMsg, "@worldmodeltype", json_string("RSGUpdate"));
	json_object_set_new(updateWaspStatusNodeMsg, "operation", json_string("UPDATE_ATTRIBUTES"));
	json_t *updateWaspNode = json_object();
	json_object_set_new(updateWaspNode, "@graphtype", json_string("Node"));
	json_object_set_new(updateWaspNode, "id", json_string(waspStatusId));

	// attributes see above

	json_object_set_new(updateWaspNode, "attributes", attributes);
	json_object_set_new(updateWaspStatusNodeMsg, "node", updateWaspNode);

	/* CReate message*/
	msg = encode_json_message(self, updateWaspStatusNodeMsg);
	/* Send the message */
	shout_message(self, msg);
	/* Wait for a reply */
	reply = wait_for_reply(self, msg, self->timeout);
	/* Print reply */
	printf("#########################################\n");
	printf("[%s] Got reply: %s \n", self->name, reply);

	/* Parse reply */
	json_t* updateWaspStatusReply = json_loads(reply, 0, &error);
	json_t* querySuccessMsg = json_object_get(updateWaspStatusReply, "updateSuccess");
	bool updateSuccess = false;
	char* dump = json_dumps(querySuccessMsg, JSON_ENCODE_ANY);
	printf("[%s] querySuccessMsg is: %s \n", self->name, dump);
	free(dump);

	if (querySuccessMsg) {
		//updateSuccess = json_is_true(querySuccessMsg);
		updateSuccess = true; //FIXME updates of same values are not yet supported
	}

	json_decref(updateWaspStatusNodeMsg);
	json_decref(updateWaspStatusReply);
	free(msg);
	free(reply);

	return updateSuccess;
}

bool add_battery(component_t *self, double battery_voltage, char* battery_status,  double utc_time_stamp_in_mili_sec, char* author) {

	if (self == NULL) {
		return false;
		printf("[ERROR] Communication component is not yet initialized.\n");
	}

	char* msg;
	char* reply;
    json_error_t error;


	/* Get root ID to restrict search to subgraph of local SWM */
	char* root_id = 0;
	if (!get_root_node_id(self, &root_id)) {
		printf("[%s] [ERROR] Cannot get root  Id \n", self->name);
		return false;
	}

	char* batteryId = 0;
	if (!get_node_by_attribute_in_subgrapgh(self, &batteryId, "sherpa:observation_type", "battery", root_id)) { // battery does not exist yet, so we will add it here

		/* Get observationGroupId */
		char* observationGroupId = 0;
		if(!get_observations_group_id(self, &observationGroupId)) {
			printf("[%s] [ERROR] Cannot get observation group Id \n", self->name);
			return false;
		}
		printf("[%s] observation Id = %s \n", self->name, observationGroupId);


		json_t *newBatteryNodeMsg = json_object();
		json_object_set_new(newBatteryNodeMsg, "@worldmodeltype", json_string("RSGUpdate"));
		json_object_set_new(newBatteryNodeMsg, "operation", json_string("CREATE"));
		json_object_set_new(newBatteryNodeMsg, "parentId", json_string(observationGroupId));
		json_t *newAgentNode = json_object();
		json_object_set_new(newAgentNode, "@graphtype", json_string("Node"));
		zuuid_t *uuid = zuuid_new ();
		const char* batteryId = zuuid_str_canonical(uuid);
		json_object_set_new(newAgentNode, "id", json_string(batteryId));

		// attributes
		json_t* attributes = json_array();
		json_t* attribute1 = json_object();
		json_object_set_new(attribute1, "key", json_string("sherpa:observation_type"));
		json_object_set_new(attribute1, "value", json_string("battery"));
		json_array_append_new(attributes, attribute1);

		json_t* attribute2 = json_object();
		json_object_set_new(attribute2, "key", json_string("sherpa:battery_voltage"));
		json_object_set_new(attribute2, "value", json_real(battery_voltage));
		json_array_append_new(attributes, attribute2);

		json_t* attribute3 = json_object();
		json_object_set_new(attribute3, "key", json_string("sherpa:battery_status"));
		json_object_set_new(attribute3, "value", json_string(battery_status));
		json_array_append_new(attributes, attribute3);

		json_object_set_new(newAgentNode, "attributes", attributes);
		json_object_set_new(newBatteryNodeMsg, "node", newAgentNode);

		/* CReate message*/
		msg = encode_json_message(self, newBatteryNodeMsg);
		/* Send the message */
		shout_message(self, msg);
		/* Wait for a reply */
		reply = wait_for_reply(self, msg, self->timeout);
		/* Print reply */
		printf("#########################################\n");
		printf("[%s] Got reply: %s \n", self->name, reply);

		/* Parse reply */
		json_t* newBatteryReply = json_loads(reply, 0, &error);
		json_t* querySuccessMsg = json_object_get(newBatteryReply, "updateSuccess");
		bool querySuccess = false;
		char* dump = json_dumps(querySuccessMsg, JSON_ENCODE_ANY);
		printf("[%s] querySuccessMsg is: %s \n", self->name, dump);
		free(dump);

		if (querySuccessMsg) {
			querySuccess = json_is_true(querySuccessMsg);
		}

		json_decref(newBatteryNodeMsg);
		json_decref(newBatteryReply);
		free(msg);
		free(reply);
		free(batteryId);
		free(uuid);

		if(!querySuccess) {
			printf("[%s] [ERROR] Can not add battery node for agent.\n", self->name);
			return false;
		}

		return true;
	}

	// if it exists already, just UPDATE the attributes

		//        batteryUpdateMsg = {
		//          "@worldmodeltype": "RSGUpdate",
		//          "operation": "UPDATE_ATTRIBUTES",
		//          "node": {
		//            "@graphtype": "Node",
		//            "id": self.battery_uuid,
		//            "attributes": [
		//                  {"key": "sensor:battery_voltage", "value": self.battery_voltage},
		//            ],
		//           },
		//        }

	json_t *updateBatteryNodeMsg = json_object();
	json_object_set_new(updateBatteryNodeMsg, "@worldmodeltype", json_string("RSGUpdate"));
	json_object_set_new(updateBatteryNodeMsg, "operation", json_string("UPDATE_ATTRIBUTES"));
	json_t *newAgentNode = json_object();
	json_object_set_new(newAgentNode, "@graphtype", json_string("Node"));
	json_object_set_new(newAgentNode, "id", json_string(batteryId));

	// attributes
	json_t* attributes = json_array();
	json_t* attribute1 = json_object();
	json_object_set_new(attribute1, "key", json_string("sherpa:observation_type"));
	json_object_set_new(attribute1, "value", json_string("battery"));
	json_array_append_new(attributes, attribute1);

	json_t* attribute2 = json_object();
	json_object_set_new(attribute2, "key", json_string("sherpa:battery_voltage"));
	json_object_set_new(attribute2, "value", json_real(battery_voltage));
	json_array_append_new(attributes, attribute2);

	json_t* attribute3 = json_object();
	json_object_set_new(attribute3, "key", json_string("sherpa:battery_status"));
	json_object_set_new(attribute3, "value", json_string(battery_status));
	json_array_append_new(attributes, attribute3);

	json_object_set_new(newAgentNode, "attributes", attributes);
	json_object_set_new(updateBatteryNodeMsg, "node", newAgentNode);

	/* CReate message*/
	msg = encode_json_message(self, updateBatteryNodeMsg);
	/* Send the message */
	shout_message(self, msg);
	/* Wait for a reply */
	reply = wait_for_reply(self, msg, self->timeout);
	/* Print reply */
	printf("#########################################\n");
	printf("[%s] Got reply: %s \n", self->name, reply);

	/* Parse reply */
	json_t* updateBatteryReply = json_loads(reply, 0, &error);
	json_t* querySuccessMsg = json_object_get(updateBatteryReply, "updateSuccess");
	bool updateSuccess = false;
	char* dump = json_dumps(querySuccessMsg, JSON_ENCODE_ANY);
	printf("[%s] querySuccessMsg is: %s \n", self->name, dump);
	free(dump);

	if (querySuccessMsg) {
		//updateSuccess = json_is_true(querySuccessMsg);
		updateSuccess = true; //FIXME updates of same values are not yet supported
	}

	json_decref(updateBatteryNodeMsg);
	json_decref(updateBatteryReply);
	free(msg);
	free(reply);

	return updateSuccess;
}

bool add_sherpa_box_status(component_t *self, sbox_status status, char* author) {

	if (self == NULL) {
		return false;
		printf("[ERROR] Communication component is not yet initialized.\n");
	}

	char* msg;
	char* reply;
    json_error_t error;


	/* Get root ID to restrict search to subgraph of local SWM */
	char* root_id = 0;
	if (!get_root_node_id(self, &root_id)) {
		printf("[%s] [ERROR] Cannot get root  Id \n", self->name);
		return false;
	}

	char* batteryId = 0;
	if (!get_node_by_attribute_in_subgrapgh(self, &batteryId, "sherpa:status_type", "sherpa_box", root_id)) { // battery does not exist yet, so we will add it here

		/* Get statusGroupId */
		char* statusGroupId = 0;
		if(!get_status_group_id(self, &statusGroupId)) {
			printf("[%s] [ERROR] Cannot get status group Id \n", self->name);
			return false;
		}
		printf("[%s] observation Id = %s \n", self->name, statusGroupId);


		json_t *newSherpaBoxStatusNodeMsg = json_object();
		json_object_set_new(newSherpaBoxStatusNodeMsg, "@worldmodeltype", json_string("RSGUpdate"));
		json_object_set_new(newSherpaBoxStatusNodeMsg, "operation", json_string("CREATE"));
		json_object_set_new(newSherpaBoxStatusNodeMsg, "parentId", json_string(statusGroupId));
		json_t *newSBoxNode = json_object();
		json_object_set_new(newSBoxNode, "@graphtype", json_string("Node"));
		zuuid_t *uuid = zuuid_new ();
		char* sherpaBoxStatusId = zuuid_str_canonical(uuid);
		json_object_set_new(newSBoxNode, "id", json_string(sherpaBoxStatusId));

		// attributes
		json_t* attributes = json_array();
		json_t* attribute1 = json_object();
		json_object_set_new(attribute1, "key", json_string("sherpa:status_type"));
		json_object_set_new(attribute1, "value", json_string("sherpa_box"));
		json_array_append_new(attributes, attribute1);

		json_t* attribute2 = json_object();
		json_object_set_new(attribute2, "key", json_string("sherpa_box:idle"));
		json_object_set_new(attribute2, "value", json_integer(status.idle));
		json_array_append_new(attributes, attribute2);

		json_t* attribute3 = json_object();
		json_object_set_new(attribute3, "key", json_string("sherpa_box:completed"));
		json_object_set_new(attribute3, "value", json_integer(status.completed));
		json_array_append_new(attributes, attribute3);

		json_t* attribute4 = json_object();
		json_object_set_new(attribute4, "key", json_string("sherpa_box:executeId"));
		json_object_set_new(attribute4, "value", json_integer(status.executeId));
		json_array_append_new(attributes, attribute4);

		json_t* attribute5 = json_object();
		json_object_set_new(attribute5, "key", json_string("sherpa_box:commandStep"));
		json_object_set_new(attribute5, "value", json_integer(status.executeId));
		json_array_append_new(attributes, attribute5);

		json_t* attribute6 = json_object();
		json_object_set_new(attribute6, "key", json_string("sherpa_box:linActuatorPosition"));
		json_object_set_new(attribute6, "value", json_integer(status.linActuatorPosition));
		json_array_append_new(attributes, attribute6);

		json_t* attribute7 = json_object();
		json_object_set_new(attribute7, "key", json_string("sherpa_box:waspDockLeft"));
		json_object_set_new(attribute7, "value", json_boolean(status.waspDockLeft));
		json_array_append_new(attributes, attribute7);

		json_t* attribute8 = json_object();
		json_object_set_new(attribute8, "key", json_string("sherpa_box:waspDockRight"));
		json_object_set_new(attribute8, "value", json_boolean(status.waspDockRight));
		json_array_append_new(attributes, attribute8);

		json_t* attribute9 = json_object();
		json_object_set_new(attribute9, "key", json_string("sherpa_box:waspLockedLeft"));
		json_object_set_new(attribute9, "value", json_boolean(status.waspLockedLeft));
		json_array_append_new(attributes, attribute9);

		json_t* attribute10 = json_object();
		json_object_set_new(attribute10, "key", json_string("sherpa_box:waspLockedRight"));
		json_object_set_new(attribute10, "value", json_boolean(status.waspLockedRight));
		json_array_append_new(attributes, attribute10);

		json_object_set_new(newSBoxNode, "attributes", attributes);
		json_object_set_new(newSherpaBoxStatusNodeMsg, "node", newSBoxNode);

		/* CReate message*/
		msg = encode_json_message(self, newSherpaBoxStatusNodeMsg);
		/* Send the message */
		shout_message(self, msg);
		/* Wait for a reply */
		reply = wait_for_reply(self, msg, self->timeout);
		/* Print reply */
		printf("#########################################\n");
		printf("[%s] Got reply: %s \n", self->name, reply);

		/* Parse reply */
		json_t* newSherpaBoxStatusReply = json_loads(reply, 0, &error);
		json_t* querySuccessMsg = json_object_get(newSherpaBoxStatusReply, "updateSuccess");
		bool querySuccess = false;
		char* dump = json_dumps(querySuccessMsg, JSON_ENCODE_ANY);
		printf("[%s] querySuccessMsg is: %s \n", self->name, dump);
		free(dump);

		if (querySuccessMsg) {
			querySuccess = json_is_true(querySuccessMsg);
		}

		json_decref(newSherpaBoxStatusNodeMsg);
		json_decref(newSherpaBoxStatusReply);
		free(msg);
		free(reply);
		free(sherpaBoxStatusId);
		free(uuid);

		if(!querySuccess) {
			printf("[%s] [ERROR] Can not add battery node for agent.\n", self->name);
			return false;
		}

		return true;
	}

	// if it exists already, just UPDATE the attributes

		//        batteryUpdateMsg = {
		//          "@worldmodeltype": "RSGUpdate",
		//          "operation": "UPDATE_ATTRIBUTES",
		//          "node": {
		//            "@graphtype": "Node",
		//            "id": self.battery_uuid,
		//            "attributes": [
		//                  {"key": "sensor:battery_voltage", "value": self.battery_voltage},
		//            ],
		//           },
		//        }

	json_t *updateSherpaBoxStatusNodeMsg = json_object();
	json_object_set_new(updateSherpaBoxStatusNodeMsg, "@worldmodeltype", json_string("RSGUpdate"));
	json_object_set_new(updateSherpaBoxStatusNodeMsg, "operation", json_string("UPDATE_ATTRIBUTES"));
	json_t *newSherpaBoxStatusNode = json_object();
	json_object_set_new(newSherpaBoxStatusNode, "@graphtype", json_string("Node"));
	json_object_set_new(newSherpaBoxStatusNode, "id", json_string(batteryId));

	// attributes
	json_t* attributes = json_array();
	json_t* attribute1 = json_object();
	json_object_set_new(attribute1, "key", json_string("sherpa:status_type"));
	json_object_set_new(attribute1, "value", json_string("sherpa_box"));
	json_array_append_new(attributes, attribute1);

	json_t* attribute2 = json_object();
	json_object_set_new(attribute2, "key", json_string("sherpa_box:idle"));
	json_object_set_new(attribute2, "value", json_integer(status.idle));
	json_array_append_new(attributes, attribute2);

	json_t* attribute3 = json_object();
	json_object_set_new(attribute3, "key", json_string("sherpa_box:completed"));
	json_object_set_new(attribute3, "value", json_integer(status.completed));
	json_array_append_new(attributes, attribute3);

	json_t* attribute4 = json_object();
	json_object_set_new(attribute4, "key", json_string("sherpa_box:executeId"));
	json_object_set_new(attribute4, "value", json_integer(status.executeId));
	json_array_append_new(attributes, attribute4);

	json_t* attribute5 = json_object();
	json_object_set_new(attribute5, "key", json_string("sherpa_box:commandStep"));
	json_object_set_new(attribute5, "value", json_integer(status.executeId));
	json_array_append_new(attributes, attribute5);

	json_t* attribute6 = json_object();
	json_object_set_new(attribute6, "key", json_string("sherpa_box:linActuatorPosition"));
	json_object_set_new(attribute6, "value", json_integer(status.linActuatorPosition));
	json_array_append_new(attributes, attribute6);

	json_t* attribute7 = json_object();
	json_object_set_new(attribute7, "key", json_string("sherpa_box:waspDockLeft"));
	json_object_set_new(attribute7, "value", json_boolean(status.waspDockLeft));
	json_array_append_new(attributes, attribute7);

	json_t* attribute8 = json_object();
	json_object_set_new(attribute8, "key", json_string("sherpa_box:waspDockRight"));
	json_object_set_new(attribute8, "value", json_boolean(status.waspDockRight));
	json_array_append_new(attributes, attribute8);

	json_t* attribute9 = json_object();
	json_object_set_new(attribute9, "key", json_string("sherpa_box:waspLockedLeft"));
	json_object_set_new(attribute9, "value", json_boolean(status.waspLockedLeft));
	json_array_append_new(attributes, attribute9);

	json_t* attribute10 = json_object();
	json_object_set_new(attribute10, "key", json_string("sherpa_box:waspLockedRight"));
	json_object_set_new(attribute10, "value", json_boolean(status.waspLockedRight));
	json_array_append_new(attributes, attribute10);

	json_object_set_new(newSherpaBoxStatusNode, "attributes", attributes);
	json_object_set_new(updateSherpaBoxStatusNodeMsg, "node", newSherpaBoxStatusNode);

	/* CReate message*/
	msg = encode_json_message(self, updateSherpaBoxStatusNodeMsg);
	/* Send the message */
	shout_message(self, msg);
	/* Wait for a reply */
	reply = wait_for_reply(self, msg, self->timeout);
	/* Print reply */
	printf("#########################################\n");
	printf("[%s] Got reply: %s \n", self->name, reply);

	/* Parse reply */
	json_t* updateSherpaBoxStatusReply = json_loads(reply, 0, &error);
	json_t* querySuccessMsg = json_object_get(updateSherpaBoxStatusReply, "updateSuccess");
	bool updateSuccess = false;
	char* dump = json_dumps(querySuccessMsg, JSON_ENCODE_ANY);
	printf("[%s] querySuccessMsg is: %s \n", self->name, dump);
	free(dump);

	if (querySuccessMsg) {
		//updateSuccess = json_is_true(querySuccessMsg);
		updateSuccess = true; //FIXME updates of same values are not yet supported
	}

	json_decref(updateSherpaBoxStatusNodeMsg);
	json_decref(updateSherpaBoxStatusReply);
	free(msg);
	free(reply);

	return updateSuccess;
}

bool add_agent(component_t *self, double* transform_matrix, double utc_time_stamp_in_mili_sec, char *agent_name) {

	if (self == NULL) {
		return false;
		printf("[ERROR] Communication component is not yet initialized.\n");
	}

	char* msg;
	char* reply;
	json_error_t error;

	/*
	 * Get the "root" node. It is relevant to specify a new pose.
	 */
	char* rootId = 0;
	if(!get_root_node_id(self, &rootId)) {
		printf("[%s] [ERROR] Cannot get root Id \n", self->name);
		return false;
	}
	printf("[%s] root Id = %s \n", self->name, rootId);

	/*
	 * Get the "origin" node. It is relevant to specify a new pose.
	 */
	char* originId = 0;
	if(!get_gis_origin_id(self, &originId)) {
		printf("[%s] [ERROR] Cannot get origin Id \n", self->name);
		return false;
	}
	printf("[%s] origin Id = %s \n", self->name, originId);


	char* agentId = 0;
	if (!get_node_by_attribute(self, &agentId, "sherpa:agent_name", agent_name)) { // agent does not exist yet, so we will add it here

		/* Get observationGroupId */
		char* agentsGroupId = 0;
		if(!get_node_by_attribute(self, &agentsGroupId, "name", "animals")) {
			printf("[%s] [ERROR] Cannot get agents  group Id \n", self->name);
			return false;
		}
		printf("[%s] observation Id = %s \n", self->name, agentsGroupId);

		/*
		 * Add a new agent
		 */

		// top level message
		json_t *newAgentNodeMsg = json_object();
		json_object_set_new(newAgentNodeMsg, "@worldmodeltype", json_string("RSGUpdate"));
		json_object_set_new(newAgentNodeMsg, "operation", json_string("CREATE"));
		json_object_set_new(newAgentNodeMsg, "parentId", json_string(agentsGroupId));
		json_t *newAgentNode = json_object();
		json_object_set_new(newAgentNode, "@graphtype", json_string("Group"));
		zuuid_t *uuid = zuuid_new ();
//		const char* new_agentId;
//		new_agentId = zuuid_str_canonical(uuid);
//		json_object_set_new(newAgentNode, "id", json_string(new_agentId));
		agentId = zuuid_str_canonical(uuid);
		json_object_set_new(newAgentNode, "id", json_string(agentId));


		// attributes
		json_t* attributes = json_array();
		json_t* attribute1 = json_object();
		json_object_set_new(attribute1, "key", json_string("sherpa:agent_name"));
		json_object_set_new(attribute1, "value", json_string(agent_name));
		json_array_append_new(attributes, attribute1);
		json_object_set_new(newAgentNode, "attributes", attributes);
		json_object_set_new(newAgentNodeMsg, "node", newAgentNode);

		/* CReate message*/
		msg = encode_json_message(self, newAgentNodeMsg);
		/* Send the message */
		shout_message(self, msg);
		/* Wait for a reply */
		reply = wait_for_reply(self, msg, self->timeout);
		/* Print reply */
		printf("#########################################\n");
		printf("[%s] Got reply: %s \n", self->name, reply);

		/* Parse reply */
		json_t* newAgentReply = json_loads(reply, 0, &error);
		json_t* querySuccessMsg = json_object_get(newAgentReply, "updateSuccess");
		bool querySuccess = false;
		char* dump = json_dumps(querySuccessMsg, JSON_ENCODE_ANY);
		printf("[%s] querySuccessMsg is: %s \n", self->name, dump);
		free(dump);

		if (querySuccessMsg) {
			querySuccess = json_is_true(querySuccessMsg);
		}

		json_decref(newAgentNodeMsg);
		json_decref(newAgentReply);
		free(msg);
		free(reply);

		if(!querySuccess) {
			printf("[%s] [ERROR] CAn nor add agent.\n", self->name);
			return false;
		}



	} // exists

	/* We will also make THIS root node a child of the agent node, so the overall structure gets more hierarchical */

//	{
//	  "@worldmodeltype": "RSGUpdate",
//	  "operation": "CREATE",
//	  "node": {
//	    "@graphtype": "Group",
//	    "id": "d0483c43-4a36-4197-be49-de829cdd66c9"
//	  },
//	  "parentId": "193db306-fd8c-4eb8-a3ab-36910665773b",
//	}
	json_t *newParentMsg = json_object();
	json_object_set_new(newParentMsg, "@worldmodeltype", json_string("RSGUpdate"));
	json_object_set_new(newParentMsg, "operation", json_string("CREATE_PARENT"));
	json_object_set_new(newParentMsg, "parentId", json_string(agentId));
	json_t *newParentNode = json_object();
	json_object_set_new(newParentNode, "@graphtype", json_string("Node"));
	json_object_set_new(newParentNode, "childId", json_string(rootId));
	json_object_set_new(newParentMsg, "node", newParentNode);

	/* Create message*/
	msg = encode_json_message(self, newParentMsg);
	/* Send the message */
	shout_message(self, msg);
	/* Wait for a reply */
	reply = wait_for_reply(self, msg, self->timeout);
	/* Print reply */
	printf("#########################################\n");
	printf("[%s] Got reply: %s \n", self->name, reply);

	/* Parse reply */
	json_t* newParentReply = json_loads(reply, 0, &error);
	json_t* querySuccessMsg = json_object_get(newParentReply, "updateSuccess");
	bool querySuccess = false;
	char* dump = json_dumps(querySuccessMsg, JSON_ENCODE_ANY);
	printf("[%s] querySuccessMsg is: %s \n", self->name, dump);
	free(dump);

	if (querySuccessMsg) {
		querySuccess = json_is_true(querySuccessMsg);
	}

	json_decref(newParentMsg);
	json_decref(newParentReply);
	free(msg);
	free(reply);

	if(!querySuccess) {
		printf("[%s] [ERROR] Can not add this WMA as part of SHERPA agent. Maybe is existed already?\n", self->name);
//		return false;
	}




	char* poseId = 0;
	char poseName[512] = {0};
	snprintf(poseName, sizeof(poseName), "%s%s", agent_name, "_geopose");
	if (!get_node_by_attribute(self, &poseId, "tf:name", poseName)) { // pose does not exist yet, so we will add it here

		/*
		 * Get the "origin" node. It is relevant to specify a new pose.
		 */
		char* originId = 0;
		if(!get_gis_origin_id(self, &originId)) {
			printf("[%s] [ERROR] Cannot get origin Id \n", self->name);
			return false;
		}
		printf("[%s] origin Id = %s \n", self->name, originId);

		/*
		 * Finally add a pose ;-)
		 */
		int mission_history_in_sec = 3600; // Up to one hour of caching.
		if(!add_geopose_to_node(self, agentId, &poseId, transform_matrix, utc_time_stamp_in_mili_sec, "tf:name", poseName, mission_history_in_sec)) {
			printf("[%s] [ERROR] Cannot add agent pose  \n", self->name);
			return false;
		}
		printf("[%s] agent pose Id = %s \n", self->name, poseId);

	} else { // update instead
		if(!update_pose(self, transform_matrix, utc_time_stamp_in_mili_sec, agent_name)) {
			printf("[%s] [ERROR] add_agent: Cannot update pose of agent  \n", self->name);
			return false;
		}
	}

	free(agentId);
	free(poseId);

	return true;
}

bool update_pose(component_t *self, double* transform_matrix, double utc_time_stamp_in_mili_sec, char *agentName) {

	if (self == NULL) {
		return false;
		printf("[ERROR] Communication component is not yet initialized.\n");
	}
	char *msg;

	/*
	 * Get ID of pose to be updates (can be made more efficient)
	 */
	json_t *getPoseIdMsg = json_object();
	json_object_set_new(getPoseIdMsg, "@worldmodeltype", json_string("RSGQuery"));
	json_object_set_new(getPoseIdMsg, "query", json_string("GET_NODES"));
	json_t *poseIdAttribute = json_object();
	json_object_set_new(poseIdAttribute, "key", json_string("tf:name"));
    char poseName[512] = {0};
    snprintf(poseName, sizeof(poseName), "%s%s", agentName, "_geopose");
	json_object_set_new(poseIdAttribute, "value", json_string(poseName));
	json_t *attributes = json_array();
	json_array_append_new(attributes, poseIdAttribute);
	//	json_object_set(attributes, "attributes", queryAttribute);
	json_object_set_new(getPoseIdMsg, "attributes", attributes);

	/* Send message and wait for reply */
	msg = encode_json_message(self, getPoseIdMsg);
	shout_message(self, msg);
	char* reply = wait_for_reply(self, msg, self->timeout);
	printf("#########################################\n");
	printf("[%s] Got reply for agent group: %s \n", self->name, reply);

	json_decref(getPoseIdMsg);

	json_error_t error;
	json_t *poseIdReply = json_loads(reply, 0, &error);
	json_t* poseIdAsJSON = 0;
	json_t* poseIdArray = json_object_get(poseIdReply, "ids");
	if (poseIdArray) {
		if( json_array_size(poseIdArray) > 0 ) {
			poseIdAsJSON = json_array_get(poseIdArray, 0);
			char* dump = json_dumps(poseIdAsJSON, JSON_ENCODE_ANY);
			printf("[%s] Pose ID is: %s \n", self->name, dump);
			free(dump);
		} else {
			printf("[%s] [ERROR] Pose does not exist!\n", self->name);
			return false;
		}
	}
	free(msg);
	free(reply);

	/*
	 * Send update
	 */

    // top level message
    json_t *newTfNodeMsg = json_object();
    json_object_set_new(newTfNodeMsg, "@worldmodeltype", json_string("RSGUpdate"));
    json_object_set_new(newTfNodeMsg, "operation", json_string("UPDATE_TRANSFORM"));
//    printf("[%s] Pose ID is: %s \n", self->name, strdup( json_dumps(poseIdAsJSON, JSON_ENCODE_ANY) ));
    json_t *newTfConnection = json_object();
    json_object_set_new(newTfConnection, "@graphtype", json_string("Connection"));
    json_object_set_new(newTfConnection, "@semanticContext", json_string("Transform"));
    json_object_set(newTfConnection, "id", poseIdAsJSON);
    // Attributes

    // history
    json_t *history = json_array();
    json_t *stampedPose = json_object();
    json_t *stamp = json_object();
    json_t *pose = json_object();

    // stamp
    json_object_set_new(stamp, "@stamptype", json_string("TimeStampUTCms"));
    json_object_set_new(stamp, "stamp", json_real(utc_time_stamp_in_mili_sec));

    //pose
    json_object_set_new(pose, "type", json_string("HomogeneousMatrix44"));
    json_object_set_new(pose, "unit", json_string("latlon"));
    json_t *matrix = json_array();
    json_t *row0 = json_array();
	json_array_append_new(row0, json_real(transform_matrix[0]));
	json_array_append_new(row0, json_real(transform_matrix[4]));
	json_array_append_new(row0, json_real(transform_matrix[8]));
	json_array_append_new(row0, json_real(transform_matrix[12]));
	json_t *row1 = json_array();
	json_array_append_new(row1, json_real(transform_matrix[1]));
	json_array_append_new(row1, json_real(transform_matrix[5]));
	json_array_append_new(row1, json_real(transform_matrix[9]));
	json_array_append_new(row1, json_real(transform_matrix[13]));
	json_t *row2 = json_array();
	json_array_append_new(row2, json_real(transform_matrix[2]));
	json_array_append_new(row2, json_real(transform_matrix[6]));
	json_array_append_new(row2, json_real(transform_matrix[10]));
	json_array_append_new(row2, json_real(transform_matrix[14]));
	json_t *row3 = json_array();
	json_array_append_new(row3, json_real(transform_matrix[3]));
	json_array_append_new(row3, json_real(transform_matrix[7]));
	json_array_append_new(row3, json_real(transform_matrix[11]));
	json_array_append_new(row3, json_real(transform_matrix[15]));
    json_array_append_new(matrix, row0);
    json_array_append_new(matrix, row1);
    json_array_append_new(matrix, row2);
    json_array_append_new(matrix, row3);

    json_object_set_new(pose, "matrix", matrix);

    json_object_set_new(stampedPose, "stamp", stamp);
    json_object_set_new(stampedPose, "transform", pose);
    json_array_append_new(history, stampedPose);
    json_object_set_new(newTfConnection, "history", history);
    json_object_set_new(newTfNodeMsg, "node", newTfConnection);


    /* Send message and wait for reply */
    msg = encode_json_message(self, newTfNodeMsg);
    shout_message(self, msg);
    reply = wait_for_reply(self, msg, self->timeout);
    printf("#########################################\n");
    printf("[%s] Got reply for pose: %s \n", self->name, reply);

    /* Clean up */
    json_decref(newTfNodeMsg);
    json_decref(poseIdReply); // this has to be deleted late, since its ID is used within other queries
    //json_decref(poseIdAsJSON);
	free(msg);
    free(reply);

    return true;
}

bool get_position(component_t *self, double* xOut, double* yOut, double* zOut, double utc_time_stamp_in_mili_sec, char *agent_name) {
	double matrix[16] = { 1, 0, 0, 0,
			               0, 1, 0, 0,
			               0, 0, 1, 0,
			               0, 0, 0, 1}; // y,x,z,1 remember this is column-majo

	bool result = get_pose(self, matrix, utc_time_stamp_in_mili_sec, agent_name);

	*xOut = matrix[12];
	*yOut = matrix[13];
	*zOut = matrix[14];

	return result;
}

bool get_pose(component_t *self, double* transform_matrix, double utc_time_stamp_in_mili_sec, char *agent_name) {
	char *msg;

	/*
	 * Get ID of agent by name
	 */
	json_t *getAgentMsg = json_object();
	json_object_set_new(getAgentMsg, "@worldmodeltype", json_string("RSGQuery"));
	json_object_set_new(getAgentMsg, "query", json_string("GET_NODES"));
	json_t *agentAttribute = json_object();
	json_object_set_new(agentAttribute, "key", json_string("sherpa:agent_name"));
	json_object_set_new(agentAttribute, "value", json_string(agent_name));
	json_t *attributes = json_array();
	json_array_append_new(attributes, agentAttribute);
	json_object_set_new(getAgentMsg, "attributes", attributes);

	/* Send message and wait for reply */
	msg = encode_json_message(self, getAgentMsg);
	shout_message(self, msg);
	char* reply = wait_for_reply(self, msg, self->timeout);
	printf("#########################################\n");
	printf("[%s] Got reply for agent group: %s \n", self->name, reply);

	free(msg);//?!?
	json_decref(getAgentMsg);

	json_error_t error;
	json_t *agentIdReply = json_loads(reply, 0, &error);
	json_t* agentIdAsJSON = 0;
	json_t* agentArray = json_object_get(agentIdReply, "ids");
	if (agentArray) {
		if( json_array_size(agentArray) > 0 ) {
			agentIdAsJSON = json_array_get(agentArray, 0);
			printf("[%s] Agent ID is: %s \n", self->name, json_dumps(json_array_get(agentArray, 0), JSON_ENCODE_ANY) );
		} else {
			printf("[%s] [ERROR] Agent does not exist. Pose query skipped.\n", self->name);
			return false;
		}
	}
	free(reply);

	/*
	 * Get origin ID
	 */
	json_t *getOriginMsg = json_object();
	json_object_set_new(getOriginMsg, "@worldmodeltype", json_string("RSGQuery"));
	json_object_set_new(getOriginMsg, "query", json_string("GET_NODES"));
	json_t * originAttribute = json_object();
	json_object_set_new(originAttribute, "key", json_string("gis:origin"));
	json_object_set_new(originAttribute, "value", json_string("wgs84"));
	attributes = json_array();
	json_array_append_new(attributes, originAttribute);
	json_object_set_new(getOriginMsg, "attributes", attributes);

	/* Send message and wait for reply */
    msg = encode_json_message(self, getOriginMsg);
    shout_message(self, msg);
    reply = wait_for_reply(self, msg, self->timeout); // TODO free older reply
    printf("#########################################\n");
    printf("[%s] Got reply: %s \n", self->name, reply);

    json_decref(getOriginMsg);

    /* Parse reply */
    json_t *originIdReply = json_loads(reply, 0, &error);
    free(msg);
    free(reply);
    json_t* originIdAsJSON = 0;
    json_t* originArray = json_object_get(originIdReply, "ids");
    if (originArray) {
    	printf("[%s] result array found. \n", self->name);
    	if( json_array_size(originArray) > 0 ) {
    		originIdAsJSON = json_array_get(originArray, 0);
        	printf("[%s] Origin ID is: %s \n", self->name, json_dumps(originIdAsJSON, JSON_ENCODE_ANY));
    	} else {
			printf("[%s] [ERROR] Origin does not exist. Pose query skipped.\n", self->name);
			return false;
		}
    }
	/*
	 * Get pose at time utc_time_stamp_in_mili_sec
	 */
//    {
//      "@worldmodeltype": "RSGQuery",
//      "query": "GET_TRANSFORM",
//      "id": "3304e4a0-44d4-4fc8-8834-b0b03b418d5b",
//      "idReferenceNode": "e379121f-06c6-4e21-ae9d-ae78ec1986a1",
//      "timeStamp": {
//        "@stamptype": "TimeStampDate",
//        "stamp": "2015-11-09T16:16:44Z"
//      }
//    }
	json_t *getTransformMsg = json_object();
	json_object_set_new(getTransformMsg, "@worldmodeltype", json_string("RSGQuery"));
	json_object_set_new(getTransformMsg, "query", json_string("GET_TRANSFORM"));
	json_object_set(getTransformMsg, "id", agentIdAsJSON);
	json_object_set(getTransformMsg, "idReferenceNode", originIdAsJSON);
	// stamp
	json_t *stamp = json_object();
	json_object_set_new(stamp, "@stamptype", json_string("TimeStampUTCms"));
	json_object_set_new(stamp, "stamp", json_real(utc_time_stamp_in_mili_sec));
	json_object_set_new(getTransformMsg, "timeStamp", stamp);

	/* Send message and wait for reply */
    msg = encode_json_message(self, getTransformMsg);
    shout_message(self, msg);
    reply = wait_for_reply(self, msg, self->timeout); // TODO free older reply
    printf("#########################################\n");
    printf("[%s] Got reply: %s \n", self->name, reply);

	/*
	 * Parse result
	 */

    /* Parse reply */
    json_t *transformReply = json_loads(reply, 0, &error);
    json_t* transform = json_object_get(transformReply, "transform");
    free(msg);
    free(reply);
    if(transform) {
    	json_t* matrix = json_object_get(transform, "matrix");
    	transform_matrix[0]  = json_real_value(json_array_get(json_array_get(matrix, 0), 0));
    	transform_matrix[4]  = json_real_value(json_array_get(json_array_get(matrix, 0), 1));
    	transform_matrix[8]  = json_real_value(json_array_get(json_array_get(matrix, 0), 2));
    	transform_matrix[12] = json_real_value(json_array_get(json_array_get(matrix, 0), 3));

    	transform_matrix[1]  = json_real_value(json_array_get(json_array_get(matrix, 1), 0));
    	transform_matrix[5]  = json_real_value(json_array_get(json_array_get(matrix, 1), 1));
    	transform_matrix[9]  = json_real_value(json_array_get(json_array_get(matrix, 1), 2));
    	transform_matrix[13] = json_real_value(json_array_get(json_array_get(matrix, 1), 3));

    	transform_matrix[2]  = json_real_value(json_array_get(json_array_get(matrix, 2), 0));
    	transform_matrix[6]  = json_real_value(json_array_get(json_array_get(matrix, 2), 1));
    	transform_matrix[10] = json_real_value(json_array_get(json_array_get(matrix, 2), 2));
    	transform_matrix[14] = json_real_value(json_array_get(json_array_get(matrix, 2), 3));

    	transform_matrix[3]  = json_real_value(json_array_get(json_array_get(matrix, 3), 0));
    	transform_matrix[7]  = json_real_value(json_array_get(json_array_get(matrix, 3), 1));
    	transform_matrix[11] = json_real_value(json_array_get(json_array_get(matrix, 3), 2));
    	transform_matrix[15] = json_real_value(json_array_get(json_array_get(matrix, 3), 3));

    } else {
    	return false;
    }

    json_decref(getTransformMsg);
    json_decref(agentIdReply); // this has to be deleted late, since its ID is used within other queries
    json_decref(originIdReply);
    json_decref(transformReply);

	return true;
}

bool get_sherpa_box_status(component_t *self, sbox_status* status, char* agent_name) {

	if (self == NULL) {
		return false;
		printf("[ERROR] Communication component is not yet initialized.\n");
	}

	char* msg;
	char* reply;
    json_error_t error;
	char* sherpa_box_status_id = 0;

    if(agent_name != 0) { // make the agent name optional, since in a mission the is typically only one SHERPA box.

		/* Get ID to restrict search to subgraph of local SWM (might be skipped for sbox, since there is only one) */
		char* scope_id = 0;
		if (!get_node_by_attribute(self, &scope_id, "sherpa:agent_name", agent_name)) { // only search within the scope of this agent
			printf("[%s] [ERROR] Cannot get scope Id \n", self->name);
			return false;
		}

		/* Find the node based on the above scope id */
		if (!get_node_by_attribute_in_subgrapgh(self, &sherpa_box_status_id, "sherpa:status_type", "sherpa_box", scope_id)) {
			printf("[%s] [ERROR] Cannot get sherpa_box_status_id \n", self->name);
			return false;
		}
    } else { // search globally. usually the cases for a SHARPA mission
		if (!get_node_by_attribute(self, &sherpa_box_status_id, "sherpa:status_type", "sherpa_box")) {
			printf("[%s] [ERROR] Cannot get sherpa_box_status_id \n", self->name);
			return false;
		}
    }

	/*
	 * Get all attributes of the node
	 * {
  	 *	 "@worldmodeltype": "RSGQuery",
  	 *	"query": "GET_NODE_ATTRIBUTES",
  	 * 	"id": "92cf7a8d-4529-4abd-b174-5fabbdd3068f"
	 * }
	 */
	json_t *getAttributesmMsg = json_object();
	json_object_set_new(getAttributesmMsg, "@worldmodeltype", json_string("RSGQuery"));
	json_object_set_new(getAttributesmMsg, "query", json_string("GET_NODE_ATTRIBUTES"));
	json_object_set_new(getAttributesmMsg, "id", json_string(sherpa_box_status_id));


	/* Send message and wait for reply */
    msg = encode_json_message(self, getAttributesmMsg);
    shout_message(self, msg);
    reply = wait_for_reply(self, msg, self->timeout);

	/*
	 * Parse result
	 */
    json_t *attributesReply = json_loads(reply, 0, &error);
    json_t* attributes = json_object_get(attributesReply, "attributes");
    size_t i;
    char* k = 0;
    for (i = 0; i < json_array_size(attributes); ++i) {

    	k = json_string_value(json_object_get(json_array_get(attributes, i), "key"));
    	if(strncmp(k, "sherpa_box:idle", sizeof("sherpa_box:idle")) == 0) {
    		status->idle = json_integer_value(json_object_get(json_array_get(attributes, i), "value"));
    		printf("[%s] SBOX status (%s = %i) \n", self->name, k, status->idle);
    	} else	if(strncmp(k, "sherpa_box:completed", sizeof("sherpa_box:completed")) == 0) {
    		status->completed = json_integer_value(json_object_get(json_array_get(attributes, i), "value"));
    		printf("[%s] SBOX status (%s = %i) \n", self->name, k, status->completed);
    	} else	if(strncmp(k, "sherpa_box:executeId", sizeof("sherpa_box:executeId")) == 0) {
    		status->executeId = json_integer_value(json_object_get(json_array_get(attributes, i), "value"));
    		printf("[%s] SBOX status (%s = %i) \n", self->name, k, status->executeId);
    	} else	if(strncmp(k, "sherpa_box:commandStep", sizeof("sherpa_box:commandStep")) == 0) {
    		status->commandStep = json_integer_value(json_object_get(json_array_get(attributes, i), "value"));
    		printf("[%s] SBOX status (%s = %i) \n", self->name, k, status->commandStep);
    	} else	if(strncmp(k, "sherpa_box:linActuatorPosition", sizeof("sherpa_box:linActuatorPosition")) == 0) {
    		status->linActuatorPosition = json_integer_value(json_object_get(json_array_get(attributes, i), "value"));
    		printf("[%s] SBOX status (%s = %i) \n", self->name, k, status->linActuatorPosition);
    	}  else	if(strncmp(k, "sherpa_box:waspDockLeft", sizeof("sherpa_box:waspDockLeft")) == 0) {
    		status->waspDockLeft = json_is_true(json_object_get(json_array_get(attributes, i), "value"));
    		printf("[%s] SBOX status (%s = %d) \n", self->name, k, status->waspDockLeft);
    	} else	if(strncmp(k, "sherpa_box:waspDockRight", sizeof("sherpa_box:waspDockRight")) == 0) {
    		status->waspDockRight = json_is_true(json_object_get(json_array_get(attributes, i), "value"));
    		printf("[%s] SBOX status (%s = %d) \n", self->name, k, status->waspDockRight);
    	} else	if(strncmp(k, "sherpa_box:waspLockedLeft", sizeof("sherpa_box:waspLockedLeft")) == 0) {
    		status->waspLockedLeft = json_is_true(json_object_get(json_array_get(attributes, i), "value"));
    		printf("[%s] SBOX status (%s = %d) \n", self->name, k, status->waspLockedLeft);
    	} else	if(strncmp(k, "sherpa_box:waspLockedRight", sizeof("sherpa_box:waspLockedRight")) == 0) {
    		status->waspLockedRight = json_is_true(json_object_get(json_array_get(attributes, i), "value"));
    		printf("[%s] SBOX status (%s = %d) \n", self->name, k, status->waspLockedRight);
    	}

	}

    json_decref(getAttributesmMsg);
    json_decref(attributesReply);
    free(msg);
    free(reply);

    return true;
}

bool get_wasp_flight_status(component_t *self, wasp_flight_status *status, char* agent_name) {

	if (self == NULL) {
		return false;
		printf("[ERROR] Communication component is not yet initialized.\n");
	}

	char* msg;
	char* reply;
	json_error_t error;
	char* waspStatusId = 0;
	if(agent_name == 0)
	{
		printf("[ERROR] agent_name is empty, return zero \n");
		return false;
	}
	if(agent_name != 0) { // make the agent name optional, since in a mission the is typically only one SHERPA box.
		// !get_node_by_attribute_in_subgrapgh(self, &waspStatusId, "sherpa:status_type", "wasp", scope_id)
		/* Get ID to restrict search to subgraph of local SWM (might be skipped for sbox, since there is only one) */
		char* scope_id = 0;
		if (!get_node_by_attribute(self, &scope_id, "sherpa:agent_name", agent_name)) { // only search within the scope of this agent
			printf("[%s] [ERROR] Cannot get scope Id \n", self->name);
			return false;
		}

		/* Find the node based on the above scope id */
		if (!get_node_by_attribute_in_subgrapgh(self, &waspStatusId, "sherpa:status_type", "wasp", scope_id)) {
			printf("[%s] [ERROR] Cannot get wasp_status_id \n", self->name);
			return false;
		}
	} else { // search globally. usually the cases for a SHARPA mission
		if (!get_node_by_attribute(self, &waspStatusId, "sherpa:status_type", "wasp")) {
			printf("[%s] [ERROR] Cannot get wasp_status_id \n", self->name);
			return false;
		}
	}

	/*
	 * Get all attributes of the node
	 * {
	 * "@worldmodeltype": "RSGQuery",
	 * "query": "GET_NODE_ATTRIBUTES",
	 * "id": "92cf7a8d-4529-4abd-b174-5fabbdd3068f"
	 * }
	 */
	json_t *getAttributesmMsg = json_object();
	json_object_set_new(getAttributesmMsg, "@worldmodeltype", json_string("RSGQuery"));
	json_object_set_new(getAttributesmMsg, "query", json_string("GET_NODE_ATTRIBUTES"));
	json_object_set_new(getAttributesmMsg, "id", json_string(waspStatusId));


	/* Send message and wait for reply */
	msg = encode_json_message(self, getAttributesmMsg);
	shout_message(self, msg);
	reply = wait_for_reply(self, msg, self->timeout);

	/*
	 * Parse result
	 */
	json_t *attributesReply = json_loads(reply, 0, &error);
	json_t* attributes = json_object_get(attributesReply, "attributes");
	size_t i;

	for (i = 0; i < json_array_size(attributes); ++i) {
		const char* k = json_string_value(json_object_get(json_array_get(attributes, i), "key"));
		if(strncmp(k, "sherpa:wasp_flight_state", sizeof("sherpa:wasp_flight_state")) == 0) {
			const char* temp = json_string_value(json_object_get(json_array_get(attributes, i), "value"));
			status->flight_state = strdup(temp);
			//printf("[%s] WASP (%s = %i) \n", self->name, k, status->idle);
		}
		//start
		//end

	}

	json_decref(getAttributesmMsg);
	json_decref(attributesReply);
	free(msg);
	free(reply);

	return true;
}

bool get_wasp_dock_status(component_t *self, wasp_dock_status *status, char* agent_name) {

	if (self == NULL) {
		return false;
		printf("[ERROR] Communication component is not yet initialized.\n");
	}

	char* msg;
	char* reply;
	json_error_t error;
	char* waspStatusId = 0;

	if(agent_name == 0)
	{
		printf("[ERROR] agent_name is empty, return zero \n");
		return false;
	}
	if(agent_name != 0) {
		// !get_node_by_attribute_in_subgrapgh(self, &waspStatusId, "sherpa:status_type", "wasp", scope_id)
		/* Get ID to restrict search to subgraph of local SWM (might be skipped for sbox, since there is only one) */
		char* scope_id = 0;
		if (!get_node_by_attribute(self, &scope_id, "sherpa:agent_name", agent_name)) { // only search within the scope of this agent
			printf("[%s] [ERROR] Cannot get scope Id \n", self->name);
			return false;
		}

		/* Find the node based on the above scope id */
		if (!get_node_by_attribute_in_subgrapgh(self, &waspStatusId, "sherpa:status_type", "wasp_docking", scope_id)) {
			printf("[%s] [ERROR] Cannot get wasp_status_id \n", self->name);
			return false;
		}
	} else { // search globally. usually the cases for a SHARPA mission
		if (!get_node_by_attribute(self, &waspStatusId, "sherpa:status_type", "wasp_docking")) {
			printf("[%s] [ERROR] Cannot get wasp_status_id \n", self->name);
			return false;
		}
	}

	/*
	 * Get all attributes of the node
	 * {
	 * "@worldmodeltype": "RSGQuery",
	 * "query": "GET_NODE_ATTRIBUTES",
	 * "id": "92cf7a8d-4529-4abd-b174-5fabbdd3068f"
	 * }
	 */
	json_t *getAttributesmMsg = json_object();
	json_object_set_new(getAttributesmMsg, "@worldmodeltype", json_string("RSGQuery"));
	json_object_set_new(getAttributesmMsg, "query", json_string("GET_NODE_ATTRIBUTES"));
	json_object_set_new(getAttributesmMsg, "id", json_string(waspStatusId));


	/* Send message and wait for reply */
	msg = encode_json_message(self, getAttributesmMsg);
	shout_message(self, msg);
	reply = wait_for_reply(self, msg, self->timeout);

	/*
	 * Parse result
	 */
	json_t *attributesReply = json_loads(reply, 0, &error);
	json_t* attributes = json_object_get(attributesReply, "attributes");
	size_t i;

	for (i = 0; i < json_array_size(attributes); ++i) {
		const char* k = json_string_value(json_object_get(json_array_get(attributes, i), "key"));
		if(strncmp(k, "sherpa:wasp_on_box", sizeof("sherpa:wasp_on_box")) == 0) {
			const char* temp = json_string_value(json_object_get(json_array_get(attributes, i), "value"));
			status->wasp_on_box = strdup(temp);
			//printf("[%s] WASP (%s = %i) \n", self->name, k, status->idle);
		}
		//start
		//end

	}
}

bool add_area(component_t *self, double *polygon_coordinates, int num_coordinates, char* area_name) {
	char* msg;
	char* reply;
	json_error_t error;

	if (self == NULL) {
		return false;
		printf("[ERROR] Communication component is not yet initialized.\n");
	}

	/* check if it exits already?!? */
	char* existing_area_id;
	if(get_node_by_attribute(self, &existing_area_id, "name", area_name)) {
		printf("[%s] [ERROR] An area with name %s exists already.\n", self->name, area_name);
		return false;
	}

	/* Get group to store polygons */
	char* environment_id;
	if(!get_node_by_attribute(self, &environment_id, "name", "enviroment")) {
		printf("[%s] [ERROR] Cannot get id for environment Group. \n", self->name);
		return false;
	}

	struct timeval tp;
	gettimeofday(&tp, NULL);
	double utcTimeInMiliSec = tp.tv_sec * 1000 + tp.tv_usec / 1000; //get current timestamp in milliseconds

	/*
	 * Every node for a point in the polygon has to be later referenced by a Connection on the field "targetIds".
	 * So we collect the ids while the node are being created.
	 * */
	json_t *targetIds = json_array();

	/*
	 * FOR each polygon add a geo point
	 */
	int i = 0;
	char* firstNode;
	for (i = 0; i < num_coordinates-1; ++i) {

		if (i== num_coordinates-1) { // last and firsts are the same; this is only reflected in the targetIds so we skip creation of the last point
			json_array_append_new(targetIds, json_string(firstNode));
		}

		/* create node */
		json_t *newPointMsg = json_object();
		json_object_set_new(newPointMsg, "@worldmodeltype", json_string("RSGUpdate"));
		json_object_set_new(newPointMsg, "operation", json_string("CREATE"));
		json_object_set_new(newPointMsg, "parentId", json_string(environment_id));
		json_t *newPointNode = json_object();
		json_object_set_new(newPointNode, "@graphtype", json_string("Node"));
		zuuid_t *uuid = zuuid_new ();
		json_object_set_new(newPointNode, "id", json_string(zuuid_str_canonical(uuid)));

		// attributes
		json_t *newPointAttributes = json_array();
		json_t *attribute1 = json_object();
		json_object_set_new(attribute1, "key", json_string("name"));
		char pointName[512] = {0};
		snprintf(pointName, sizeof(pointName), "%s%s%i", area_name, "_point_", i);
		json_array_append_new(newPointAttributes, attribute1);
		json_object_set_new(attribute1, "value", json_string(pointName));
		json_object_set_new(newPointNode, "attributes", newPointAttributes);
		json_object_set_new(newPointMsg, "node", newPointNode);

		/* Send message and wait for reply */
		msg = encode_json_message(self, newPointMsg);
		shout_message(self, msg);
		reply = wait_for_reply(self, msg, self->timeout);

		/* Parse reply */
		json_t* replyAsJSON = json_loads(reply, 0, &error);
		free(msg);
		free(reply);

		json_t* querySuccessMsg = json_object_get(replyAsJSON, "updateSuccess");
		bool querySuccess = false;
		if (querySuccessMsg) {
			querySuccess = json_is_true(querySuccessMsg);
		}


		json_decref(newPointMsg);
		json_decref(replyAsJSON);
		json_decref(querySuccessMsg);

		if(!querySuccess) {
			printf("[%s] [ERROR] Cannot add polygon point. \n", self->name);
			return false;
		}

		/* add geopose to node */
		char* poseId;
		double matrix[16] = { 1, 0, 0, 0,
				               0, 1, 0, 0,
				               0, 0, 1, 0,
				               0, 0, 0, 1}; // y,x,z,1 remember this is column-major!
		matrix[12] = polygon_coordinates[2*i];
		matrix[13] = polygon_coordinates[(2*i)+1];
		if(!add_geopose_to_node(self, zuuid_str_canonical(uuid), &poseId, matrix, utcTimeInMiliSec, 0, 0, 10)) {
			printf("[%s] [ERROR] Cannot add polygon coordinates. \n", self->name);
			return false;
		}

		/* store id for connection */
		json_array_append_new(targetIds, json_string(zuuid_str_canonical(uuid))); // ID of node that we just have created before


		if(i==0) {
			firstNode = strdup(zuuid_str_canonical(uuid));
		}

		free(poseId);
	}

	/*
	 * Finally we explicitly reference the above created nodes by a Connection.
	 */
//	{
//	  "@worldmodeltype": "RSGUpdate",
//	  "operation": "CREATE",
//	  "node": {
//
//	    "@graphtype": "Connection",
//	    "id": "8ce59f8e-6072-49c0-a0fc-481ee288e24b",
//	    "attributes": [
//	      {"key": "geo:area", "value": "polygon"},
//	    ],
//	    "sourceIds": [
//	    ],
//	    "targetIds": [
//	      "7a47e674-f3c3-47d9-aae3-fd558603076b",
//	      "2743bbaa-a590-42b7-aa38-d573c18fe6f6",
//	      "e2bb6580-a15f-4f90-84cc-a0696b031294",
//	      "bb54d653-4e8f-47a7-af9b-b0f935a181e2",
//	      "7a47e674-f3c3-47d9-aae3-fd558603076b",
//	    ],
//	    "start": { "@stamptype": "TimeStampUTCms" , "stamp": 0.0 },
//	    "end": { "@stamptype": "TimeStampDate" , "stamp": "2020-00-00T00:00:00Z" }
//
//	  },
//	  "parentId": "e379121f-06c6-4e21-ae9d-ae78ec1986a1",
//	}
	json_t *newAreaConnectionMsg = json_object();
	json_object_set_new(newAreaConnectionMsg, "@worldmodeltype", json_string("RSGUpdate"));
	json_object_set_new(newAreaConnectionMsg, "operation", json_string("CREATE"));
	json_object_set_new(newAreaConnectionMsg, "parentId", json_string(environment_id));

	json_t *newAreaConnection = json_object();
	json_object_set_new(newAreaConnection, "@graphtype", json_string("Connection"));
	zuuid_t *areaUuid = zuuid_new();
	json_object_set_new(newAreaConnection, "id", json_string(areaUuid));

	// Attributes
	json_t *attribute1 = json_object();
	json_object_set_new(attribute1, "key", json_string("geo:area"));
	json_object_set_new(attribute1, "value", json_string("polygon"));
	json_t *poseAttributes = json_array();
	json_array_append_new(poseAttributes, attribute1);
	json_t *attribute2 = json_object();
	json_object_set_new(attribute2, "key", json_string("name"));
	json_object_set_new(attribute2, "value", json_string(area_name));
	json_array_append_new(poseAttributes, attribute2);
	json_object_set_new(newAreaConnection, "attributes", poseAttributes);

	// sourceIds
	json_t *sourceIds = json_array();
	json_object_set_new(newAreaConnection, "sourceIds", sourceIds);

	// sourceIds
	json_object_set_new(newAreaConnection, "targetIds", targetIds); // add  above collected ids here

	json_object_set_new(newAreaConnectionMsg, "node", newAreaConnection);

	/* Send message and wait for reply */
	msg = encode_json_message(self, newAreaConnectionMsg);
	shout_message(self, msg);
	reply = wait_for_reply(self, msg, self->timeout);

	/* Parse reply */
	json_t* replyAsJSON = json_loads(reply, 0, &error);
	free(msg);
	free(reply);

	json_t* querySuccessMsg = json_object_get(replyAsJSON, "updateSuccess");
	bool querySuccess = false;
	if (querySuccessMsg) {
		querySuccess = json_is_true(querySuccessMsg);
	}

	json_decref(newAreaConnectionMsg);
	json_decref(replyAsJSON);
	json_decref(querySuccessMsg);
	free(areaUuid);


	return querySuccess;
}


bool load_dem(component_t *self, char* map_file_name) {
	char* msg;
	char* reply;
	json_error_t error;

	if (self == NULL) {
		return false;
		printf("[ERROR] Communication component is not yet initialized.\n");
	}

	// derive path to FBX (function blocks) modules
	char* fbxPath = getenv("FBX_MODULES");
	char path[512] = {0};
	snprintf(path, sizeof(path), "%s%s", fbxPath, "/lib");

	/* Load demloader block */
//	{
//	  "@worldmodeltype": "RSGFunctionBlock",
//	  "metamodel":       "rsg-functionBlock-schema.json",
//	  "name":            "demloader",
//	  "operation":       "LOAD",
//	  "input": {
//	  "metamodel": "rsg-functionBlock-path-schema.json",
//	    "path":     "/opt/src/sandbox/brics_3d_function_blocks/lib/",
//	    "comment":  "path is the same as the FBX_MODULES environment variable appended with a lib/ folder"
//	  }
//	}


	json_t *loadDemBlockMsg = json_object();
	json_object_set_new(loadDemBlockMsg, "@worldmodeltype", json_string("RSGFunctionBlock"));
	json_object_set_new(loadDemBlockMsg, "metamodel", json_string("rsg-functionBlock-schema.json"));
	json_object_set_new(loadDemBlockMsg, "name", json_string("demloader"));
	json_object_set_new(loadDemBlockMsg, "operation", json_string("LOAD"));
	json_t *functioBlockInput = json_object();
	json_object_set_new(functioBlockInput, "metamodel", json_string("rsg-functionBlock-path-schema.json"));
	json_object_set_new(functioBlockInput, "path", json_string(path));
	json_object_set_new(loadDemBlockMsg, "input", functioBlockInput);

	/* Send message and wait for reply */
	msg = encode_json_message(self, loadDemBlockMsg);
	shout_message(self, msg);
	reply = wait_for_reply(self, msg, self->timeout);

	/* Parse reply */
	json_t* replyAsJSON = json_loads(reply, 0, &error);
	free(msg);
	free(reply);

	json_t* operationSuccessMsg = json_object_get(replyAsJSON, "operationSuccess");
	bool operationSuccess = false;
	if (operationSuccessMsg) {
		operationSuccess = json_is_true(operationSuccessMsg);
	}

	json_decref(loadDemBlockMsg);
	json_decref(replyAsJSON);
	json_decref(operationSuccessMsg);

	if(!operationSuccess) {
		printf("[%s] [ERROR] Cannot load dem block \n", self->name);
		return false;
	}



	/* Load map */
//	{
//	  "@worldmodeltype": "RSGFunctionBlock",
//	  "metamodel":       "rsg-functionBlock-schema.json",
//	  "name":            "demloader",
//	  "operation":       "EXECUTE",
//	  "input": {
//	    "metamodel": "fbx-demloader-input-schema.json",
//	    "command":  "LOAD_MAP",
//	    "file":     "examples/maps/dem/davos.tif"
//	  }
//	}

	// top level message
	json_t *loadMapMsg = json_object();
	json_object_set_new(loadMapMsg, "@worldmodeltype", json_string("RSGFunctionBlock"));
	json_object_set_new(loadMapMsg, "metamodel", json_string("rsg-functionBlock-schema.json"));
	json_object_set_new(loadMapMsg, "name", json_string("demloader"));
	json_object_set_new(loadMapMsg, "operation", json_string("EXECUTE"));
	json_t *mapInput = json_object();
	json_object_set_new(mapInput, "metamodel", json_string("fbx-demloader-input-schema.json"));
	json_object_set_new(mapInput, "command", json_string("LOAD_MAP"));
	json_object_set_new(mapInput, "file", json_string(map_file_name));
	json_object_set_new(loadMapMsg, "input", mapInput);

	/* Send message and wait for reply */
	msg = encode_json_message(self, loadMapMsg);
	shout_message(self, msg);
	reply = wait_for_reply(self, msg, self->timeout);

	/* Parse reply */
	replyAsJSON = json_loads(reply, 0, &error);
	free(msg);
	free(reply);

	operationSuccessMsg = json_object_get(replyAsJSON, "operationSuccess");
	operationSuccess = false;
	if (operationSuccessMsg) {
		operationSuccess = json_is_true(operationSuccessMsg);
	}

	json_decref(loadMapMsg);
	json_decref(replyAsJSON);
	json_decref(operationSuccessMsg);

	if(!operationSuccess) {
		printf("[%s] [ERROR] Cannot load dem map \n", self->name);
		return false;
	}

	return true;
}

bool get_elevataion_at(component_t *self, double* elevation, double latitude, double longitude) {
	char* msg;
	char* reply;
	json_error_t error;

	if (self == NULL) {
		return false;
		printf("[ERROR] Communication component is not yet initialized.\n");
	}

	*elevation = -1.0;

	/* Get elevation */
//	{
//	  "@worldmodeltype": "RSGFunctionBlock",
//	  "metamodel":       "rsg-functionBlock-schema.json",
//	  "name":            "demloader",
//	  "operation":       "EXECUTE",
//	  "input": {
//	    "metamodel": "fbx-demloader-input-schema.json",
//	    "command":   "GET_ELEVATION",
//	    "latitude":  9.849468,
//	    "longitude": 46.812785
//	  }
//	}

	json_t *getElevationMsg = json_object();
	json_object_set_new(getElevationMsg, "@worldmodeltype", json_string("RSGFunctionBlock"));
	json_object_set_new(getElevationMsg, "metamodel", json_string("rsg-functionBlock-schema.json"));
	json_object_set_new(getElevationMsg, "name", json_string("demloader"));
	json_object_set_new(getElevationMsg, "operation", json_string("EXECUTE"));
	json_t *functioBlockInput = json_object();
	json_object_set_new(functioBlockInput, "metamodel", json_string("fbx-demloader-input-schema.json"));
	json_object_set_new(functioBlockInput, "command", json_string("GET_ELEVATION"));
	json_object_set_new(functioBlockInput, "latitude", json_real(latitude));
	json_object_set_new(functioBlockInput, "longitude", json_real(longitude));
	json_object_set_new(getElevationMsg, "input", functioBlockInput);

	/* Send message and wait for reply */
	msg = encode_json_message(self, getElevationMsg);
	shout_message(self, msg);
	reply = wait_for_reply(self, msg, self->timeout);

	/* Parse reply */
	json_t* replyAsJSON = json_loads(reply, 0, &error);
	free(msg);
	free(reply);

	json_t* operationSuccessMsg = json_object_get(replyAsJSON, "operationSuccess");
	bool operationSuccess = false;
	if (operationSuccessMsg) {
		operationSuccess = json_is_true(operationSuccessMsg);
	}

	if(operationSuccess) {
		json_t* output = json_object_get(replyAsJSON, "output");
		if(output) {
			*elevation = json_real_value(json_object_get(output, "elevation"));
		}
	}

	json_decref(getElevationMsg);
	json_decref(replyAsJSON);
	json_decref(operationSuccessMsg);

	if(!operationSuccess) {
		printf("[%s] [ERROR] Cannot get elevation value. \n", self->name);
		return false;
	}

	return true;
}

bool get_min_max_elevation_in_area(component_t *self, double* min_elevation, double* max_elevation, char* area_name) {
	char* msg;
	char* reply;
	json_error_t error;

	if (self == NULL) {
		return false;
		printf("[ERROR] Communication component is not yet initialized.\n");
	}

	*min_elevation = -1.0;
	*max_elevation = -1.0;


	char* area_id;
	if(!get_node_by_attribute(self, &area_id, "name", area_name)) {
		printf("[%s] [ERROR] An area with name does not exist.\n", area_name, self->name);
		return false;
	}



	/* Get elevation */
//	{
//	  "@worldmodeltype": "RSGFunctionBlock",
//	  "metamodel":       "rsg-functionBlock-schema.json",
//	  "name":            "demloader",
//	  "operation":       "EXECUTE",
//	  "input": {
//	    "metamodel": "fbx-demloader-input-schema.json",
//	    "command":   "GET_MIN_MAX_ELEVATION",
//	    "areaId":    "8ce59f8e-6072-49c0-a0fc-481ee288e24b"
//	  }
//	}

	json_t *getElevationMsg = json_object();
	json_object_set_new(getElevationMsg, "@worldmodeltype", json_string("RSGFunctionBlock"));
	json_object_set_new(getElevationMsg, "metamodel", json_string("rsg-functionBlock-schema.json"));
	json_object_set_new(getElevationMsg, "name", json_string("demloader"));
	json_object_set_new(getElevationMsg, "operation", json_string("EXECUTE"));
	json_t *functioBlockInput = json_object();
	json_object_set_new(functioBlockInput, "metamodel", json_string("fbx-demloader-input-schema.json"));
	json_object_set_new(functioBlockInput, "command", json_string("GET_MIN_MAX_ELEVATION"));
	json_object_set_new(functioBlockInput, "areaId", json_string(area_id));
	json_object_set_new(getElevationMsg, "input", functioBlockInput);

	/* Send message and wait for reply */
	msg = encode_json_message(self, getElevationMsg);
	shout_message(self, msg);
	reply = wait_for_reply(self, msg, self->timeout);

	/* Parse reply */
	json_t* replyAsJSON = json_loads(reply, 0, &error);
	free(msg);
	free(reply);

	json_t* operationSuccessMsg = json_object_get(replyAsJSON, "operationSuccess");
	bool operationSuccess = false;
	if (operationSuccessMsg) {
		operationSuccess = json_is_true(operationSuccessMsg);
	}

	if(operationSuccess) {
		json_t* output = json_object_get(replyAsJSON, "output");
		if(output) {
			*min_elevation = json_real_value(json_object_get(output, "minElevation"));
			*max_elevation = json_real_value(json_object_get(output, "maxElevation"));
		}
	}

	json_decref(getElevationMsg);
	json_decref(replyAsJSON);
	json_decref(operationSuccessMsg);

	if(!operationSuccess) {
		printf("[%s] [ERROR] Cannot get elevation min/max values. \n", self->name);
		return false;
	}


	return true;
}
