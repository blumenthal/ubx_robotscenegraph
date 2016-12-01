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
    	zyre_stop (self->local);
		printf ("[%s] Stopping zyre node.\n", self->name);
		zclock_sleep (100);
		zyre_destroy (&self->local);
		printf ("[%s] Destroying component.\n", self->name);
        zyre_destroy (&self->local);
        json_decref(self->config);
        zpoller_destroy (&self->poller);
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

component_t* new_component(json_t *config) {
	component_t *self = (component_t *) zmalloc (sizeof (component_t));
    if (!self)
        return NULL;
    if (!config)
            return NULL;

    self->config = config;

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

	int rc;
	if(!json_is_null(json_object_get(config, "gossip_endpoint"))) {
		//  Set up gossip network for this node
		zyre_gossip_connect (self->local, "%s", json_string_value(json_object_get(config, "gossip_endpoint")));
		printf("[%s] using gossip with gossip hub '%s' \n", self->name,json_string_value(json_object_get(config, "gossip_endpoint")));
	} else {
		printf("[%s] WARNING: no local gossip communication is set! \n", self->name);
	}
	rc = zyre_start (self->local);
	assert (rc == 0);
	self->localgroup = strdup("local");
	zyre_join (self->local, self->localgroup);
	//  Give time for them to connect
	zclock_sleep (1000);

	//create a list to store queries...
	self->query_list = zlist_new();
	if ((!self->query_list)&&(zlist_size (self->query_list) == 0)) {
		destroy_component (&self);
		return NULL;
	}

	self->alive = 1; //will be used to quit program after answer to query is received
	self->poller =  zpoller_new (zyre_socket(self->local), NULL);
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
    	result->payload = /*strdup(*/json_dumps(json_object_get(root, "payload"), JSON_ENCODE_ANY)/*)*/;
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
		printf("[%s] send_json_message: No queryId found, adding one.\n", self->name);
		zuuid_t *uuid = zuuid_new ();
		assert(uuid);
		char* uuid_as_string = zuuid_str_canonical(uuid);
	    json_object_set_new(pl, "queryId", json_string(uuid_as_string));
	    free(uuid_as_string);
	    free(uuid);
	}

	char* query_id = json_string_value(json_object_get(pl,"queryId"));
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

char* wait_for_reply(component_t* self) {
	char* ret = 0;

    // timestamp for timeout
    struct timespec ts = {0,0};
    struct timespec curr_time = {0,0};
    if (clock_gettime(CLOCK_MONOTONIC,&ts)) {
		printf("[%s] Could not assign time stamp!\n",self->name);
	}
    if (clock_gettime(CLOCK_MONOTONIC,&curr_time)) {
		printf("[%s] Could not assign time stamp!\n",self->name);
	}

	while (zlist_size (self->query_list) > 0){
			//printf("[%s] Queries in queue: %d \n",self->name,zlist_size (self->query_list));
			void *which = zpoller_wait (self->poller, ZMQ_POLL_MSEC);
			if (which) {
				zmsg_t *msg = zmsg_recv (which);
				if (!msg) {
					printf("[%s] interrupted!\n", self->name);
					return -1;
				}
				//zmsg_print(msg); printf("msg end\n");
				char *event = zmsg_popstr (msg);
				if (streq (event, "ENTER")) {
					handle_enter (self, msg);
				} else if (streq (event, "EXIT")) {
					handle_exit (self, msg);
				} else if (streq (event, "SHOUT")) {
					handle_shout (self, msg, &ret);
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
			//check for timeout
			if (!clock_gettime(CLOCK_MONOTONIC,&curr_time)) {
				// if timeout, stop component
				double curr_time_msec = curr_time.tv_sec*1.0e3 +curr_time.tv_nsec*1.0e-6;
				double ts_msec = ts.tv_sec*1.0e3 +ts.tv_nsec*1.0e-6;
				if (curr_time_msec - ts_msec > self->timeout) {
					printf("[%s] Timeout! No query answer received.\n",self->name);
					destroy_component(&self);
					return 0;
				}
			} else {
				printf ("[%s] could not get current time\n", self->name);
			}
		}

	//printf("[%s] wait_for_reply received answer to query %s:\n", self->name, ret);
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

void handle_shout(component_t *self, zmsg_t *msg, char** reply) {
	assert (zmsg_size(msg) == 4);
	char *peerid = zmsg_popstr (msg);
	char *name = zmsg_popstr (msg);
	char *group = zmsg_popstr (msg);
	char *message = zmsg_popstr (msg);
	printf ("[%s] SHOUT %s %s %s %s\n", self->name, peerid, name, group, message);
	json_msg_t *result = (json_msg_t *) zmalloc (sizeof (json_msg_t));
	if (decode_json(message, result) == 0) {
//		printf ("[%s] message type %s\n", self->name, result->type);
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
						*reply = strdup(result->payload);
//						free(it->msg->payload);
						query_t *dummy = it;
						it = zlist_next(self->query_list);
						zlist_remove(self->query_list,dummy);
						query_destroy(&dummy);
					    json_decref(payload);
						break;
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
						*reply = strdup(result->payload);
						query_t *dummy = it;
						it = zlist_next(self->query_list);
						zlist_remove(self->query_list,dummy);
						query_destroy(&dummy);
					    json_decref(payload);
						break;
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
						*reply = strdup(result->payload);
						query_t *dummy = it;
						it = zlist_next(self->query_list);
						zlist_remove(self->query_list,dummy);
						query_destroy(&dummy);
					    json_decref(payload);
						break;
					}
				}
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
						*reply = strdup(result->payload);
						query_t *dummy = it;
						it = zlist_next(self->query_list);
						zlist_remove(self->query_list,dummy);
						query_destroy(&dummy);
					    json_decref(payload);
						break;
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
	reply = wait_for_reply(self);
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
//	*observations_id = NULL;
	return get_node_by_attribute(self, observations_id, "name", "observations"); //TODO only search in subgraph of local root node
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
	reply = wait_for_reply(self);
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
	reply = wait_for_reply(self);
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

bool add_geopose_to_node(component_t *self, char* node_id, char** new_geopose_id, double* transform_matrix, double utc_time_stamp_in_mili_sec, const char* key, const char* value) {
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
	reply = wait_for_reply(self);
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
	reply = wait_for_reply(self);
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
	reply = wait_for_reply(self);
	/* Print reply */
	printf("#########################################\n");
	printf("[%s] Got reply: %s \n", self->name, reply);


	char* poseId;
	bool succsess = add_geopose_to_node(self, zuuid_str_canonical(uuid), &poseId, transform_matrix, utc_time_stamp_in_mili_sec, 0, 0);


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
	reply = wait_for_reply(self);
	/* Print reply */
	printf("#########################################\n");
	printf("[%s] Got reply: %s \n", self->name, reply);


	char* poseId;
	bool succsess = add_geopose_to_node(self, zuuid_str_canonical(uuid), &poseId, transform_matrix, utc_time_stamp_in_mili_sec, 0, 0);


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
	reply = wait_for_reply(self);
	/* Print reply */
	printf("#########################################\n");
	printf("[%s] Got reply: %s \n", self->name, reply);


	char* poseId;
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
		char* batteryId = zuuid_str_canonical(uuid);
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
		reply = wait_for_reply(self);
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
	reply = wait_for_reply(self);
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
		updateSuccess = json_is_true(querySuccessMsg);
	}

	json_decref(updateBatteryNodeMsg);
	json_decref(updateBatteryReply);
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
		json_object_set_new(newAgentNode, "@graphtype", json_string("Node"));
		zuuid_t *uuid = zuuid_new ();
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
		reply = wait_for_reply(self);
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


	char* poseId = 0;
	char poseName[512] = {0};
	snprintf(poseName, sizeof(poseName), "%s%s", agent_name, "_geopose");
	if (!get_node_by_attribute(self, &poseId, "name", poseName)) { // pose does not exist yet, so we will add it here

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
		if(!add_geopose_to_node(self, agentId, &poseId, transform_matrix, utc_time_stamp_in_mili_sec, "name", poseName)) {
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
	json_object_set_new(poseIdAttribute, "key", json_string("name"));
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
	char* reply = wait_for_reply(self);
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
    reply = wait_for_reply(self);
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

bool get_position(component_t *self, double* xOut, double* yOut, double* zOut, double utc_time_stamp_in_mili_sec, char *agentName) {
	char *msg;

	/*
	 * Get ID of agent by name
	 */
	json_t *getAgentMsg = json_object();
	json_object_set_new(getAgentMsg, "@worldmodeltype", json_string("RSGQuery"));
	json_object_set_new(getAgentMsg, "query", json_string("GET_NODES"));
	json_t *agentAttribute = json_object();
	json_object_set_new(agentAttribute, "key", json_string("sherpa:agent_name"));
	json_object_set_new(agentAttribute, "value", json_string(agentName));
	json_t *attributes = json_array();
	json_array_append_new(attributes, agentAttribute);
	json_object_set_new(getAgentMsg, "attributes", attributes);

	/* Send message and wait for reply */
	msg = encode_json_message(self, getAgentMsg);
	shout_message(self, msg);
	char* reply = wait_for_reply(self);
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
    reply = wait_for_reply(self); // TODO free older reply
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
    reply = wait_for_reply(self); // TODO free older reply
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
    	*xOut = json_real_value(json_array_get(json_array_get(matrix, 0), 3));
    	*yOut = json_real_value(json_array_get(json_array_get(matrix, 1), 3));
    	*zOut = json_real_value(json_array_get(json_array_get(matrix, 2), 3));

    } else {

    	return false;
    }

    json_decref(getTransformMsg);
    json_decref(agentIdReply); // this has to be deleted late, since its ID is used within other queries
    json_decref(originIdReply);
    json_decref(transformReply);

	return true;
}
