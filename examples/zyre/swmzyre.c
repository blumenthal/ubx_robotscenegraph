#include <zyre.h>
#include <jansson.h>
#include <uuid/uuid.h>
#include <string.h>
#include "swmzyre.h"

void query_destroy (query_t **self_p) {
        assert (self_p);
        if(*self_p) {
            query_t *self = *self_p;
            free (self);
            *self_p = NULL;
        }
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
    printf("[%s] config file: %s\n", json_string_value(json_object_get(root, "short-name")), json_dumps(root, JSON_ENCODE_ANY));
    if(!root) {
   	printf("Error parsing JSON file! line %d: %s\n", error.line, error.text);
    	return NULL;
    }
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
	if(json_object_get(pl,"queryId") == 0) { // no queryIt in message, so we skip it here
		printf("send_json_message No queryId found, adding one.\n");
		zuuid_t *uuid = zuuid_new ();
		assert(uuid);
	    json_object_set(pl, "queryId", json_string(zuuid_str_canonical(uuid)));
	    json_decref(uuid);
	}

	char* query_id = json_string_value(json_object_get(pl,"queryId"));
	//printf("[%s] send_json_message: query_id = %s:\n", self->name, query_id);

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
	query_t * q = query_new(query_id, zyre_uuid(self->local), msg, NULL);
	zlist_append(self->query_list, q);

    char* ret = json_dumps(env, JSON_ENCODE_ANY);
	printf("[%s] send_json_message: message = %s:\n", self->name, ret);

	json_decref(env);
    json_decref(pl);
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
						query_t *dummy = it;
						it = zlist_next(self->query_list);
						zlist_remove(self->query_list,dummy);
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
						printf("Skipping RSGQueryResult message without queryId");
						break;
					}
					if (streq(it->uid,json_string_value(json_object_get(payload,"queryId")))) {
						printf("[%s] received answer to query %s of type %s:\n Query:\n %s\n Result:\n %s \n", self->name,it->uid,result->type,it->msg->payload, result->payload);
						*reply = strdup(result->payload);
						query_t *dummy = it;
						it = zlist_next(self->query_list);
						zlist_remove(self->query_list,dummy);
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
						printf("Skipping RSGFunctionBlockResult message without queryId");
						break;
					}
					if (streq(it->uid,json_string_value(json_object_get(payload,"queryId")))) {
						printf("[%s] received answer to query %s of type %s:\n Query:\n %s\n Result:\n %s ", self->name,it->uid,result->type,it->msg->payload, result->payload);
						*reply = strdup(result->payload);
						query_t *dummy = it;
						it = zlist_next(self->query_list);
						zlist_remove(self->query_list,dummy);
						break;
					}
				}
			}
		} else {
			printf("[%s] Unknown msg type!",self->name);
		}
	} else {
		printf ("[%s] message could not be decoded\n", self->name);
	}
	free(result);
	zstr_free(&peerid);
	zstr_free(&name);
	zstr_free(&group);
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

bool add_victim(component_t *self, double x, double y , double z, double utcTimeStampInMiliSec, char* author) {

	if (self == NULL) {
		return false;
		printf("[ERROR] Communication component is not yet initialized.\n");
	}

	char *msg;

//		json_t* tmpString = 0;
//	   /* Load configuration file for communication setup */
////	    char config_folder[255] = { SWM_ZYRE_CONFIG_DIR };
////	    char config_name[] = "swm_zyre_config.json";
////	    char config_file[512] = {0};
////	    snprintf(config_file, sizeof(config_file), "%s/%s", config_folder, config_name);
//
//	    json_t * config = load_config_file(configFile);//"swm_zyre_config.json");
//	    if (config == NULL) {
//	      return -1;
//	    }
//
//	    /* Spawn new communication component */
//	    component_t *self = new_component(config);
//	    if (self == NULL) {
//	    	return -1;
//	    }
//	    printf("[%s] component initialized!\n", self->name);
//	    char *msg;

	    /* Get observationGroupId */
	//    getObservationGroup = {
	//      "@worldmodeltype": "RSGQuery",
	//      "query": "GET_NODES",
	//      "attributes": [
	//          {"key": "name", "value": "observations"},
	//      ]
	//    }
		json_t* tmpString = 0;
		json_t *getObservationGroupMsg = json_object();
		tmpString = json_string("RSGQuery");
		json_object_set(getObservationGroupMsg, "@worldmodeltype", tmpString);
		json_decref(tmpString);
		json_object_set(getObservationGroupMsg, "query", json_string("GET_NODES"));
		json_t *queryAttribute = json_object();
		json_object_set(queryAttribute, "key", json_string("name"));
		json_object_set(queryAttribute, "value", json_string("observations"));
		json_t *attributes = json_array();
		json_array_append(attributes, queryAttribute);
		json_object_set(attributes, "attributes", queryAttribute);
		json_object_set(getObservationGroupMsg, "attributes", attributes);

		/* Send message and wait for reply */
	    msg = encode_json_message(self, getObservationGroupMsg);
	    shout_message(self, msg);
	    char* reply = wait_for_reply(self);
	    printf("#########################################\n");
	    printf("[%s] Got reply: %s \n", self->name, reply);

	    json_decref(queryAttribute);
	    json_decref(attributes);
	    json_decref(getObservationGroupMsg);

	    json_error_t error;
	    json_t *observationGroupIdReply = json_loads(reply, 0, &error);
	//    printf("[%s] message : %s\n", self->name , json_dumps(observationGroupIdReply, JSON_ENCODE_ANY));
	    char * observationGroupId;
	    json_t* observationGroupIdAsJSON = 0;
	    json_t* array = json_object_get(observationGroupIdReply, "ids");
	    if (array) {
	//    	printf("[%s] result array found: \n", self->name);
	    	if( json_array_size(array) > 0 ) {
	    		observationGroupIdAsJSON = json_array_get(array, 0);
	        	observationGroupId = strdup( json_dumps(json_array_get(array, 0), JSON_ENCODE_ANY) );
	        	printf("[%s] ID is: %s \n", self->name, observationGroupId);
	    	}
	    }

	    if(!observationGroupIdAsJSON) {
	    	printf("[%s] [ERROR] Cannot get observation group Id \n", self->name);
	    	return -1;
	    }

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
		json_object_set(newImageNodeMsg, "@worldmodeltype", json_string("RSGUpdate"));
		json_object_set(newImageNodeMsg, "operation", json_string("CREATE"));
		json_object_set(newImageNodeMsg, "parentId", observationGroupIdAsJSON);
		json_t *newImageNode = json_object();
		json_object_set(newImageNode, "@graphtype", json_string("Node"));
		zuuid_t *uuid = zuuid_new ();
		json_object_set(newImageNode, "id", json_string(zuuid_str_canonical(uuid)));

		// attributes
		attributes = json_array();
		json_t *attribute1 = json_object();
		json_object_set(attribute1, "key", json_string("sherpa:observation_type"));
		json_object_set(attribute1, "value", json_string("victim"));
		json_array_append(attributes, attribute1);
	//	json_t *attribute2 = json_object();
	//	json_object_set(attribute2, "key", json_string("sherpa:uri"));
	//	json_object_set(attribute2, "value", json_string("TODO"));
	//	json_array_append(attributes, attribute2);
		json_t *attribute3 = json_object();
		json_object_set(attribute3, "key", json_string("sherpa:stamp"));
		json_object_set(attribute3, "value", json_real(utcTimeStampInMiliSec));
		json_array_append(attributes, attribute3);
		json_t *attribute4 = json_object();
		json_object_set(attribute4, "key", json_string("sherpa:author"));
		json_object_set(attribute4, "value", json_string("hawk"));
		json_array_append(attributes, attribute4);


		json_object_set(newImageNode, "attributes", attributes);
		json_object_set(newImageNodeMsg, "node", newImageNode);

		/* CReate message*/
	    msg = encode_json_message(self, newImageNodeMsg);
	    /* Send the message */
	    shout_message(self, msg);
	    /* Wait for a reply */
	     reply = wait_for_reply(self);
	    /* Print reply */
	    printf("#########################################\n");
	    printf("[%s] Got reply: %s \n", self->name, reply);

		// Clean up
	    json_decref(attributes);
	    json_decref(attribute1);
	//    json_decref(attribute2);
	    json_decref(attribute3);
	    json_decref(attribute4);
	    json_decref(newImageNodeMsg);
	    json_decref(newImageNode);
	    json_decref(uuid);


	    /*
	     * Get the "origin" node. It is relevant to specify a new pose.
	     */
		json_t *getOriginMsg = json_object();
		json_object_set(getOriginMsg, "@worldmodeltype", json_string("RSGQuery"));
		json_object_set(getOriginMsg, "query", json_string("GET_NODES"));
		json_t * originAttribute = json_object();
		json_object_set(originAttribute, "key", json_string("gis:origin"));
		json_object_set(originAttribute, "value", json_string("wgs84"));
		attributes = json_array();
		json_array_append(attributes, originAttribute);
		json_object_set(getOriginMsg, "attributes", attributes);

		/* Send message and wait for reply */
	    msg = encode_json_message(self, getOriginMsg);
	    shout_message(self, msg);
	    reply = wait_for_reply(self); // TODO free older reply
	    printf("#########################################\n");
	    printf("[%s] Got reply: %s \n", self->name, reply);

	    /* Parse reply */
	    json_t *originIdReply = json_loads(reply, 0, &error);
	    json_t* originIdAsJSON = 0;
	    array = json_object_get(originIdReply, "ids");
	    if (array) {
	    	printf("[%s] result array found: %s \n", self->name);
	    	if( json_array_size(array) > 0 ) {
	    		originIdAsJSON = json_array_get(array, 0);
	        	printf("[%s] ID is: %s \n", self->name, json_dumps(originIdAsJSON, JSON_ENCODE_ANY));
	    	}
	    }

	    /* Clean up */
	    json_decref(attributes);
	    json_decref(originAttribute);
	    json_decref(getOriginMsg);

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
		json_object_set(newTfNodeMsg, "@worldmodeltype", json_string("RSGUpdate"));
		json_object_set(newTfNodeMsg, "operation", json_string("CREATE"));
		json_object_set(newTfNodeMsg, "parentId", observationGroupIdAsJSON);
		json_t *newTfConnection = json_object();
		json_object_set(newTfConnection, "@graphtype", json_string("Connection"));
		json_object_set(newTfConnection, "@semanticContext", json_string("Transform"));
		zuuid_t *poseUuid = zuuid_new ();
		json_object_set(newTfConnection, "id", json_string(zuuid_str_canonical(poseUuid)));
		// Attributes
		json_t *poseAttribute = json_object();
	    json_object_set(poseAttribute, "key", json_string("tf:type"));
	    json_object_set(poseAttribute, "value", json_string("wgs84"));
		json_t *poseAttributes = json_array();
		json_array_append(poseAttributes, poseAttribute);
		json_object_set(newTfConnection, "attributes", poseAttributes);
		// sourceIds
		json_t *sourceIds = json_array();
		json_array_append(sourceIds, originIdAsJSON); // ID of origin node
		json_object_set(newTfConnection, "sourceIds", sourceIds);
		// sourceIds
		json_t *targetIds = json_array();
		json_array_append(targetIds, json_string(zuuid_str_canonical(uuid))); // ID of node that we just have created before
		json_object_set(newTfConnection, "targetIds", targetIds);

		// history
		json_t *history = json_array();
		json_t *stampedPose = json_object();
		json_t *stamp = json_object();
		json_t *pose = json_object();

		// stamp
		json_object_set(stamp, "@stamptype", json_string("TimeStampUTCms"));
	//    struct timespec curr_time = {0,0};
	//    clock_gettime(CLOCK_MONOTONIC,&curr_time); // TODO check for utc time; e.g. from the ROS header. To be tested!
	//    double ts_msec = curr_time.tv_sec*1.0e3 +curr_time.tv_nsec*1.0e-6;
		json_object_set(stamp, "stamp", json_real(utcTimeStampInMiliSec));

		//pose
		json_object_set(pose, "type", json_string("HomogeneousMatrix44"));
		json_object_set(pose, "unit", json_string("latlon"));
		json_t *matrix = json_array();
		json_t *row0 = json_array();
		json_array_append(row0, json_real(1));
		json_array_append(row0, json_real(0));
		json_array_append(row0, json_real(0));
		json_array_append(row0, json_real(x));
		json_t *row1 = json_array();
		json_array_append(row1, json_real(0));
		json_array_append(row1, json_real(1));
		json_array_append(row1, json_real(0));
		json_array_append(row1, json_real(y));
		json_t *row2 = json_array();
		json_array_append(row2, json_real(0));
		json_array_append(row2, json_real(0));
		json_array_append(row2, json_real(1));
		json_array_append(row2, json_real(z));
		json_t *row3 = json_array();
		json_array_append(row3, json_real(0));
		json_array_append(row3, json_real(0));
		json_array_append(row3, json_real(0));
		json_array_append(row3, json_real(1));
		json_array_append(matrix, row0);
		json_array_append(matrix, row1);
		json_array_append(matrix, row2);
		json_array_append(matrix, row3);

		json_object_set(pose, "matrix", matrix);

		json_object_set(stampedPose, "stamp", stamp);
		json_object_set(stampedPose, "transform", pose);
		json_array_append(history, stampedPose);
		json_object_set(newTfConnection, "history", history);
		json_object_set(newTfNodeMsg, "node", newTfConnection);

		/* Send message and wait for reply */
	    msg = encode_json_message(self, newTfNodeMsg);
	    shout_message(self, msg);
	    reply = wait_for_reply(self); // TODO free older reply
	    printf("#########################################\n");
	    printf("[%s] Got reply for pose: %s \n", self->name, reply);

	    /* Clean up */
	    json_decref(newTfNodeMsg);
	    json_decref(newTfConnection);
	    json_decref(targetIds);
	    json_decref(sourceIds);
	    json_decref(poseAttribute);
	    json_decref(poseAttributes);
	    json_decref(stampedPose);
	    json_decref(stamp);
	    json_decref(history);
	    json_decref(matrix);
	    json_decref(row0);
	    json_decref(row1);
	    json_decref(row2);
	    json_decref(row3);

	    return true;
}

bool add_agent(component_t *self, double x, double y, double z, double utcTimeStampInMiliSec, char *agentName) {

	if (self == NULL) {
		return false;
		printf("[ERROR] Communication component is not yet initialized.\n");
	}
	char *msg;

	/*
	 * Check if an agent exist already
	 */
	json_t *getAgentMsg = json_object();
	json_object_set(getAgentMsg, "@worldmodeltype", json_string("RSGQuery"));
	json_object_set(getAgentMsg, "query", json_string("GET_NODES"));
	json_t *agentAttribute = json_object();
	json_object_set(agentAttribute, "key", json_string("sherpa:agent_name"));
	json_object_set(agentAttribute, "value", json_string(agentName));
	json_t *attributes = json_array();
	json_array_append(attributes, agentAttribute);
	//	json_object_set(attributes, "attributes", queryAttribute);
	json_object_set(getAgentMsg, "attributes", attributes);

	/* Send message and wait for reply */
	msg = encode_json_message(self, getAgentMsg);
	shout_message(self, msg);
	char* reply = wait_for_reply(self);
	printf("#########################################\n");
	printf("[%s] Got reply for agent group: %s \n", self->name, reply);

//	json_decref(attributes);
//	json_decref(agentAttribute);
//	json_decref(getAgentMsg);


	json_error_t error;
	json_t *agentIdReply = json_loads(reply, 0, &error);
	json_t* agentIdAsJSON = 0;
	json_t* agentArray = json_object_get(agentIdReply, "ids");
	if (agentArray) {
		if( json_array_size(agentArray) > 0 ) {
			agentIdAsJSON = json_array_get(agentArray, 0);
			printf("[%s] Agent ID is: %s \n", self->name, strdup( json_dumps(json_array_get(agentArray, 0), JSON_ENCODE_ANY) ));
			printf("[%s] Agent exists. Skipping creation of it\n", self->name);
			return false;
		}
	}




	/*
	 * Agent(animals) group
	 */
	json_t *getAgentsGroupMsg = json_object();
	json_object_set(getAgentsGroupMsg, "@worldmodeltype", json_string("RSGQuery"));
	json_object_set(getAgentsGroupMsg, "query", json_string("GET_NODES"));
	json_t *agentGroupAttribute = json_object();
	json_object_set(agentGroupAttribute, "key", json_string("name"));
	json_object_set(agentGroupAttribute, "value", json_string("animals"));
	attributes = json_array();
	json_array_append(attributes, agentGroupAttribute);
	json_object_set(attributes, "attributes", agentGroupAttribute);
	json_object_set(getAgentsGroupMsg, "attributes", attributes);

	/* Send message and wait for reply */
	msg = encode_json_message(self, getAgentsGroupMsg);
	shout_message(self, msg);
	reply = wait_for_reply(self);
	printf("#########################################\n");
	printf("[%s] Got reply for agent group: %s \n", self->name, reply);

	json_decref(attributes);
	json_decref(agentGroupAttribute);
	json_decref(getAgentsGroupMsg);

	json_t *agentGroupIdReply = json_loads(reply, 0, &error);
	json_t* agentGroupIdAsJSON = 0;
	json_t* array = json_object_get(agentGroupIdReply, "ids");
	if (array) {
		if( json_array_size(array) > 0 ) {
			agentGroupIdAsJSON = json_array_get(array, 0);
			printf("[%s] Agent group ID is: %s \n", self->name, strdup( json_dumps(json_array_get(array, 0), JSON_ENCODE_ANY) ));
		}
	}

	if(!agentGroupIdAsJSON) {
		printf("[%s] [ERROR] Cannot get observation group Id \n", self->name);
		return false;
	}


    /*
	     * Get the "origin" node. It is relevant to specify a new pose.
	     */
		json_t *getOriginMsg = json_object();
		json_object_set(getOriginMsg, "@worldmodeltype", json_string("RSGQuery"));
		json_object_set(getOriginMsg, "query", json_string("GET_NODES"));
		json_t * originAttribute = json_object();
		json_object_set(originAttribute, "key", json_string("gis:origin"));
		json_object_set(originAttribute, "value", json_string("wgs84"));
		attributes = json_array();
		json_array_append(attributes, originAttribute);
		json_object_set(getOriginMsg, "attributes", attributes);

		/* Send message and wait for reply */
	    msg = encode_json_message(self, getOriginMsg);
	    shout_message(self, msg);
	    reply = wait_for_reply(self); // TODO free older reply
	    printf("#########################################\n");
	    printf("[%s] Got reply: %s \n", self->name, reply);

	    /* Parse reply */
	    json_t *originIdReply = json_loads(reply, 0, &error);
	    json_t* originIdAsJSON = 0;
	    array = json_object_get(originIdReply, "ids");
	    if (array) {
	    	printf("[%s] result array found: %s \n", self->name);
	    	if( json_array_size(array) > 0 ) {
	    		originIdAsJSON = json_array_get(array, 0);
	        	printf("[%s] ID is: %s \n", self->name, json_dumps(originIdAsJSON, JSON_ENCODE_ANY));
	    	}
	    }

	    /* Clean up */
	    json_decref(attributes);
	    json_decref(originAttribute);
	    json_decref(getOriginMsg);


	    /*
	     * Add a new agent
	     */

	    // top level message
	    json_t *newAgentNodeMsg = json_object();
	    json_object_set(newAgentNodeMsg, "@worldmodeltype", json_string("RSGUpdate"));
	    json_object_set(newAgentNodeMsg, "operation", json_string("CREATE"));
	    json_object_set(newAgentNodeMsg, "parentId", agentGroupIdAsJSON);
	    json_t *newAgentNode = json_object();
	    json_object_set(newAgentNode, "@graphtype", json_string("Node"));
	    zuuid_t *uuid = zuuid_new ();
	    json_object_set(newAgentNode, "id", json_string(zuuid_str_canonical(uuid)));

	    // attributes
	    attributes = json_array();
	    json_t *attribute1 = json_object();
	    json_object_set(attribute1, "key", json_string("sherpa:agent_name"));
	    json_object_set(attribute1, "value", json_string(agentName));
	    json_array_append(attributes, attribute1);
	    json_object_set(newAgentNode, "attributes", attributes);
	    json_object_set(newAgentNodeMsg, "node", newAgentNode);

	    /* CReate message*/
	    msg = encode_json_message(self, newAgentNodeMsg);
	    /* Send the message */
	    shout_message(self, msg);
	    /* Wait for a reply */
	    reply = wait_for_reply(self);
	    /* Print reply */
	    printf("#########################################\n");
	    printf("[%s] Got reply: %s \n", self->name, reply);

	    // Clean up
//	    json_decref(attributes);
//	    json_decref(attribute1);
//	    json_decref(newAgentNodeMsg);
//	    json_decref(newAgentNode);
//	    json_decref(uuid);

	    /*
	     * Finally add a pose ;-)
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
	    json_object_set(newTfNodeMsg, "@worldmodeltype", json_string("RSGUpdate"));
	    json_object_set(newTfNodeMsg, "operation", json_string("CREATE"));
	    json_object_set(newTfNodeMsg, "parentId", originIdAsJSON);
	    json_t *newTfConnection = json_object();
	    json_object_set(newTfConnection, "@graphtype", json_string("Connection"));
	    json_object_set(newTfConnection, "@semanticContext", json_string("Transform"));
	    zuuid_t *poseUuid = zuuid_new ();
	    json_object_set(newTfConnection, "id", json_string(zuuid_str_canonical(poseUuid)));
	    // Attributes
	    json_t *poseAttribute = json_object();
	    json_object_set(poseAttribute, "key", json_string("tf:type"));
	    json_object_set(poseAttribute, "value", json_string("wgs84"));
	    json_t *poseAttribute2 = json_object();
	    json_object_set(poseAttribute2, "key", json_string("name"));
	    char poseName[512] = {0};
	    snprintf(poseName, sizeof(poseName), "%s%s", agentName, "_geopose");
	    json_object_set(poseAttribute2, "value", json_string(poseName));
	    json_t *poseAttributes = json_array();
	    json_array_append(poseAttributes, poseAttribute);
	    json_array_append(poseAttributes, poseAttribute2);
	    json_object_set(newTfConnection, "attributes", poseAttributes);
	    // sourceIds
	    json_t *sourceIds = json_array();
	    json_array_append(sourceIds, originIdAsJSON); // ID of origin node
	    json_object_set(newTfConnection, "sourceIds", sourceIds);
	    // sourceIds
	    json_t *targetIds = json_array();
	    json_array_append(targetIds, json_string(zuuid_str_canonical(uuid))); // ID of node that we just have created before
	    json_object_set(newTfConnection, "targetIds", targetIds);

	    // history
	    json_t *history = json_array();
	    json_t *stampedPose = json_object();
	    json_t *stamp = json_object();
	    json_t *pose = json_object();

	    // stamp
	    json_object_set(stamp, "@stamptype", json_string("TimeStampUTCms"));
	    //    struct timespec curr_time = {0,0};
	    //    clock_gettime(CLOCK_MONOTONIC,&curr_time); // TODO check for utc time; e.g. from the ROS header. To be tested!
	    //    double ts_msec = curr_time.tv_sec*1.0e3 +curr_time.tv_nsec*1.0e-6;
	    json_object_set(stamp, "stamp", json_real(utcTimeStampInMiliSec));

	    //pose
	    json_object_set(pose, "type", json_string("HomogeneousMatrix44"));
	    json_object_set(pose, "unit", json_string("latlon"));
	    json_t *matrix = json_array();
	    json_t *row0 = json_array();
	    json_array_append(row0, json_real(1));
	    json_array_append(row0, json_real(0));
	    json_array_append(row0, json_real(0));
	    json_array_append(row0, json_real(x));
	    json_t *row1 = json_array();
	    json_array_append(row1, json_real(0));
	    json_array_append(row1, json_real(1));
	    json_array_append(row1, json_real(0));
	    json_array_append(row1, json_real(y));
	    json_t *row2 = json_array();
	    json_array_append(row2, json_real(0));
	    json_array_append(row2, json_real(0));
	    json_array_append(row2, json_real(1));
	    json_array_append(row2, json_real(z));
	    json_t *row3 = json_array();
	    json_array_append(row3, json_real(0));
	    json_array_append(row3, json_real(0));
	    json_array_append(row3, json_real(0));
	    json_array_append(row3, json_real(1));
	    json_array_append(matrix, row0);
	    json_array_append(matrix, row1);
	    json_array_append(matrix, row2);
	    json_array_append(matrix, row3);

	    json_object_set(pose, "matrix", matrix);

	    json_object_set(stampedPose, "stamp", stamp);
	    json_object_set(stampedPose, "transform", pose);
	    json_array_append(history, stampedPose);
	    json_object_set(newTfConnection, "history", history);
	    json_object_set(newTfNodeMsg, "node", newTfConnection);

	    /* Send message and wait for reply */
	    msg = encode_json_message(self, newTfNodeMsg);
	    shout_message(self, msg);
	    reply = wait_for_reply(self); // TODO free older reply
	    printf("#########################################\n");
	    printf("[%s] Got reply for pose: %s \n", self->name, reply);

	    return true;
}

bool update_pose(component_t *self, double x, double y, double z, double utcTimeStampInMiliSec, char *agentName) {

	if (self == NULL) {
		return false;
		printf("[ERROR] Communication component is not yet initialized.\n");
	}
	char *msg;

	/*
	 * Get ID of pose to be updates (can be made more efficient)
	 */
	json_t *getPoseIdMsg = json_object();
	json_object_set(getPoseIdMsg, "@worldmodeltype", json_string("RSGQuery"));
	json_object_set(getPoseIdMsg, "query", json_string("GET_NODES"));
	json_t *poseIdAttribute = json_object();
	json_object_set(poseIdAttribute, "key", json_string("name"));
    char poseName[512] = {0};
    snprintf(poseName, sizeof(poseName), "%s%s", agentName, "_geopose");
	json_object_set(poseIdAttribute, "value", json_string(poseName));
	json_t *attributes = json_array();
	json_array_append(attributes, poseIdAttribute);
	//	json_object_set(attributes, "attributes", queryAttribute);
	json_object_set(getPoseIdMsg, "attributes", attributes);

	/* Send message and wait for reply */
	msg = encode_json_message(self, getPoseIdMsg);
	shout_message(self, msg);
	char* reply = wait_for_reply(self);
	printf("#########################################\n");
	printf("[%s] Got reply for agent group: %s \n", self->name, reply);

//	json_decref(attributes);
//	json_decref(poseIdAttribute);
//	json_decref(getPoseIdMsg);


	json_error_t error;
	json_t *poseIdReply = json_loads(reply, 0, &error);
	json_t* poseIdAsJSON = 0;
	json_t* poseIdArray = json_object_get(poseIdReply, "ids");
	if (poseIdArray) {
		if( json_array_size(poseIdArray) > 0 ) {
			poseIdAsJSON = json_array_get(poseIdArray, 0);
			printf("[%s] Pose ID is: %s \n", self->name, strdup( json_dumps(json_array_get(poseIdArray, 0), JSON_ENCODE_ANY) ));
		} else {
			printf("[%s] [ERROR] Pose does not exist!\n", self->name);
			return false;
		}
	}
	free(reply);

	/*
	 * Send update
	 */

    // top level message
    json_t *newTfNodeMsg = json_object();
    json_object_set(newTfNodeMsg, "@worldmodeltype", json_string("RSGUpdate"));
    json_object_set(newTfNodeMsg, "operation", json_string("UPDATE_TRANSFORM"));
    json_object_set(newTfNodeMsg, "parentId", poseIdAsJSON);
    json_t *newTfConnection = json_object();
    json_object_set(newTfConnection, "@graphtype", json_string("Connection"));
    json_object_set(newTfConnection, "@semanticContext", json_string("Transform"));
    zuuid_t *poseUuid = zuuid_new ();
    json_object_set(newTfConnection, "id", poseIdAsJSON);
    // Attributes

    // history
    json_t *history = json_array();
    json_t *stampedPose = json_object();
    json_t *stamp = json_object();
    json_t *pose = json_object();

    // stamp
    json_object_set(stamp, "@stamptype", json_string("TimeStampUTCms"));
    //    struct timespec curr_time = {0,0};
    //    clock_gettime(CLOCK_MONOTONIC,&curr_time); // TODO check for utc time; e.g. from the ROS header. To be tested!
    //    double ts_msec = curr_time.tv_sec*1.0e3 +curr_time.tv_nsec*1.0e-6;
    json_object_set(stamp, "stamp", json_real(utcTimeStampInMiliSec));

    //pose
    json_object_set(pose, "type", json_string("HomogeneousMatrix44"));
    json_object_set(pose, "unit", json_string("latlon"));
    json_t *matrix = json_array();
    json_t *row0 = json_array();
    json_array_append(row0, json_real(1));
    json_array_append(row0, json_real(0));
    json_array_append(row0, json_real(0));
    json_array_append(row0, json_real(x));
    json_t *row1 = json_array();
    json_array_append(row1, json_real(0));
    json_array_append(row1, json_real(1));
    json_array_append(row1, json_real(0));
    json_array_append(row1, json_real(y));
    json_t *row2 = json_array();
    json_array_append(row2, json_real(0));
    json_array_append(row2, json_real(0));
    json_array_append(row2, json_real(1));
    json_array_append(row2, json_real(z));
    json_t *row3 = json_array();
    json_array_append(row3, json_real(0));
    json_array_append(row3, json_real(0));
    json_array_append(row3, json_real(0));
    json_array_append(row3, json_real(1));
    json_array_append(matrix, row0);
    json_array_append(matrix, row1);
    json_array_append(matrix, row2);
    json_array_append(matrix, row3);

    json_object_set(pose, "matrix", matrix);

    json_object_set(stampedPose, "stamp", stamp);
    json_object_set(stampedPose, "transform", pose);
    json_array_append(history, stampedPose);
    json_object_set(newTfConnection, "history", history);
    json_object_set(newTfNodeMsg, "node", newTfConnection);


    /* Send message and wait for reply */
    msg = encode_json_message(self, newTfNodeMsg);
    shout_message(self, msg);
    reply = wait_for_reply(self); // TODO free older reply
    printf("#########################################\n");
    printf("[%s] Got reply for pose: %s \n", self->name, reply);

    /* Clean up */
//    json_decref(newTfNodeMsg);
//    json_decref(newTfConnection);
//    json_decref(stampedPose);
//    json_decref(stamp);
//    json_decref(history);
//    json_decref(matrix);
//    json_decref(row0);
//    json_decref(row1);
//    json_decref(row2);
//    json_decref(row3);

    return true;
}

