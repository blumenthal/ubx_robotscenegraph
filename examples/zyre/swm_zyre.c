/**
 * Example on how the send RSG-JSON messages via zyre.
 * It uses the swmzyre library as helper.
 */

#include <zyre.h>
#include <jansson.h>
#include <uuid/uuid.h>
#include <string.h>

#include "swmzyre.h"

int main(int argc, char *argv[]) {

	/* Version check */
    int major, minor, patch;
    zyre_version (&major, &minor, &patch);
    assert (major == ZYRE_VERSION_MAJOR);
    assert (minor == ZYRE_VERSION_MINOR);
    assert (patch == ZYRE_VERSION_PATCH);

    /* Load configuration file for communication setup */
    char config_folder[255] = { SWM_ZYRE_CONFIG_DIR };
    char config_name[] = "swm_zyre_config.json";
    char config_file[512] = {0};
    snprintf(config_file, sizeof(config_file), "%s/%s", config_folder, config_name);

    json_t * config = load_config_file(&config_file);//"swm_zyre_config.json");
    if (config == NULL) {
      return -1;
    }

    /* Spawn new communication component */
    component_t *self = new_component(config);
    if (self == NULL) {
    	return -1;
    }
    printf("[%s] component initialized!\n", self->name);

    /* Check if there is at least one component connected */
    zlist_t * tmp = zlist_new ();
    assert (tmp);
    assert (zlist_size (tmp) == 0);
    // timestamp for timeout
    struct timespec ts = {0,0};
    if (clock_gettime(CLOCK_MONOTONIC,&ts)) {
		printf("[%s] Could not assign time stamp!\n",self->name);
	}
    struct timespec curr_time = {0,0};
    while (true) {
    	printf("[%s] Checking for connected peers.\n",self->name);
    	tmp = zyre_peers(self->local);
    	printf("[%s] %zu peers connected.\n", self->name,zlist_size(tmp));
    	if (zlist_size (tmp) > 0)
    		break;
		if (!clock_gettime(CLOCK_MONOTONIC,&curr_time)) {
			// if timeout, stop component
			double curr_time_msec = curr_time.tv_sec*1.0e3 +curr_time.tv_nsec*1.0e-6;
			double ts_msec = ts.tv_sec*1.0e3 +ts.tv_nsec*1.0e-6;
			if (curr_time_msec - ts_msec > self->timeout) {
				printf("[%s] Timeout! Could not connect to other peers.\n",self->name);
				destroy_component(&self);
				return 0;
			}
		} else {
			printf ("[%s] could not get current time\n", self->name);
		}
    	zclock_sleep (1000);
    }
    zlist_destroy(&tmp);

    char *msg;
    if (argc == 2) {
        printf("\n");
    	printf("#########################################\n");
    	printf("[%s] Sending Generic Query\n",self->name);
    	printf("#########################################\n");
    	printf("\n");

    	msg = send_json_message(self, argv[1]);


    } else {

		/* Query */
		printf("\n");
		printf("#########################################\n");
		printf("[%s] Sending Query for RSG Root Node\n",self->name);
		printf("#########################################\n");
		printf("\n");
		json_t* query_params = NULL;
		msg = send_query(self,"GET_ROOT_NODE",query_params);
    }

    zyre_shouts(self->local, self->localgroup, "%s", msg);
	printf("[%s] Sent msg: %s \n",self->name,msg);
	if (clock_gettime(CLOCK_MONOTONIC,&ts)) {
		printf("[%s] Could not assign time stamp!\n",self->name);
	}
	//wait for query to be answered
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
				handle_shout (self, msg);
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

	return 0;


/**TODO:
 * * test why there is no query result with sebastian
 * * wait for return of query result to know root node
 * * send some updates
 * * send some some queries
 * * send some fnction_block_calls
 * * store all these queries
 * * wait till all queries are resolved
 * * make sure that some return errors
 *
 * ===============
 * Missing:
 * * set self->RSG_parent from initial root node query
 * * function to create fnction_block_calls msg
 * * rest of main running till all queries are served or timeout
 */




	int i;
    for( i = 0; i < self->no_of_updates; i++){
		printf("\n");
		printf("#########################################\n");
		printf("[%s] Sending Update %d\n",self->name,i);
		printf("#########################################\n");
		printf("\n");

		// Example update msg
		//		{
		//		  "@worldmodeltype": "RSGUpdate", <- done by function
		//		  "operation": "CREATE", <- done by function
		//		  "node": {
		//			  "@graphtype": "Node",
		//			  "id": "8f3ba6f4-5c70-46ec-83af-0d5434953e5f",
		//			  "attributes": [
		//				{"key": "name", "value": "robot_1"},
		//				{"key": "comment", "value": "none"},
		//			  ],
		//			},
		//		  },
		//		  "parentId": "193db306-fd8c-4eb8-a3ab-36910665773b",
		//		}
		json_t* update_params = json_object();
		json_t* node = json_object();
		json_t* attributes = json_array();
		json_t* attribute1 = json_object();
		json_object_set(attribute1,"key",json_string("name"));
		json_object_set(attribute1,"value",json_string("robot_1"));
		json_t* attribute2 = json_object();
		json_object_set(attribute2,"key",json_string("comment"));
		json_object_set(attribute2,"value",json_string("none"));
		json_array_append(attributes, attribute1);
		json_array_append(attributes, attribute2);
		zuuid_t *node_id = zuuid_new ();
		assert(node_id);
		json_object_set(node,"@graphtype",json_string("Node"));
		json_object_set(node,"id",json_string(zuuid_str_canonical(node_id)));
		json_object_set(node,"attributes",attributes);
		json_object_set(update_params,"node",node);
		json_object_set(update_params,"parentId",json_string(self->RSG_parent));
		char *msg = send_update(self, "CREATE",update_params);
		if (msg) {
			zyre_shouts(self->local, self->localgroup, "%s", msg);
			printf("[%s] Sent msg: %s \n",self->name,msg);
			zstr_free(&msg);
		} else {
			printf("[%s] Could not generate RSG update msg.\n",self->name);
		}
		free(msg);
		json_decref(attributes);
		json_decref(node);
		json_decref(update_params);
	}
	
//	for( i = 0; i < no_of_queries; i++){
//		printf("\n");
//		printf("#########################################\n");
//		printf("[%s] Sending Query %d\n",self,i);
//		printf("#########################################\n");
//		printf("\n");
////		char *msg = send_query();
//		char *msg = send_msg();
//		if (msg) {
//			zyre_shouts(local, localgroup, "%s", msg);
//			printf("[%s] Sent msg: %s \n",self,msg);
//			zstr_free(&msg);
//		} else {
//			alive = false;
//		}
//		free(msg);
//	}
	
//	for( i = 0; i < no_of_fcn_block_calls; i++){
//		printf("\n");
//		printf("#########################################\n");
//		printf("[%s] Sending Function Block Call %d\n",self,i);
//		printf("#########################################\n");
//		printf("\n");
////		char *msg = send_query();
//		char *msg = send_msg();
//		if (msg) {
//			zyre_shouts(local, localgroup, "%s", msg);
//			printf("[%s] Sent msg: %s \n",self,msg);
//			zstr_free(&msg);
//		} else {
//			alive = false;
//		}
//		free(msg);
//	}


//     	void *which = zpoller_wait (poller, ZMQ_POLL_MSEC);
//     	if (which) {
// 			printf("[%s] local data received!\n", self);
// 			zmsg_t *msg = zmsg_recv (which);
// 			if (!msg) {
// 				printf("[%s] interrupted!\n", self);
// 				return -1;
// 			}
// 			//reset timeout
// 			if (clock_gettime(CLOCK_MONOTONIC,&ts)) {
// 				printf("[%s] Could not assign time stamp!\n",self);
// 			}
// 			char *event = zmsg_popstr (msg);
// 			
// 			if (streq (event, "WHISPER")) {
//                 assert (zmsg_size(msg) == 3);
//                 char *peerid = zmsg_popstr (msg);
//                 char *name = zmsg_popstr (msg);
//                 char *message = zmsg_popstr (msg);
//                 printf ("[%s] %s %s %s %s\n", self, event, peerid, name, message);
//                 //printf("[%s] Received: %s from %s\n",self, event, name);
// 				json_msg_t *result = (json_msg_t *) zmalloc (sizeof (json_msg_t));
// 				if (decode_json(message, result) == 0) {
// 					// load the payload as json
// 					json_t *payload;
// 					json_error_t error;
// 					payload= json_loads(result->payload,0,&error);
// 					if(!payload) {
// 						printf("Error parsing JSON send_remote! line %d: %s\n", error.line, error.text);
// 					} else {
// 						const char *uid = json_string_value(json_object_get(payload,"UID"));
// 						//TODO:does this string need to be freed?
// 						if (!uid){
// 							printf("[%s] Received msg without UID!\n", self);
// 						} else {
// 							// search through stored list of queries and check if this query corresponds to one we have sent
// 							query_t *it = zlist_first(query_list);
// 							int found_UUID = 0;
// 							while (it != NULL) {
// 								if streq(it->UID, uid) {
// 									printf("[%s] Received reply to query %s.\n", self, uid);
// 									if (streq(result->type,"peer-list")){
// 										printf("Received peer list: %s\n",result->payload);
// 										//TODO: search list for a wasp
// 										alive = 0;
// 
// 									} else if (streq(result->type,"communication_report")){
// 										printf("Received communication_report: %s\n",result->payload);
// 										/////////////////////////////////////////////////
// 										//Do something with the report
// 										if (json_is_true(json_object_get(payload,"success"))){
// 											printf("Yeay! All recipients have received the msg.\n");
// 										} else {
// 											printf("Sending msg was not successful because of: %s\n",json_string_value(json_object_get(payload,"error")));
// 										}
// 										/////////////////////////////////////////////////
// 
// 										if (streq(json_string_value(json_object_get(payload,"error")),"Unknown recipients")){
// 											//This is really not how coordination in a program should be done -> TODO: clean up
// 						
// 										}
// 
// 									}
// 									found_UUID = 1;
// 									zlist_remove(query_list,it);
// 									//TODO: make sure the data of that query is properly freed
// 								}
// 								it = zlist_next(query_list);
// 							}
// 							if (found_UUID == 0) {
// 								printf("[%s] Received a msg with an unknown UID!\n", self);
// 							}
// 						}
// 						json_decref(payload);
// 					}
// 
// 
// 
// 				} else {
// 					printf ("[%s] message could not be decoded\n", self);
// 				}
// 				free(result);
// 				zstr_free(&peerid);
// 				zstr_free(&name);
// 				zstr_free(&message);
// 			} else {
// 				printf ("[%s] received %s msg\n", self, event);
// 			}
// 			zstr_free (&event);
// 			zmsg_destroy (&msg);
//     	} else {
// 			if (!clock_gettime(CLOCK_MONOTONIC,&curr_time)) {
// 				// if timeout, stop component
// 				double curr_time_msec = curr_time.tv_sec*1.0e3 +curr_time.tv_nsec*1.0e-6;
// 				double ts_msec = ts.tv_sec*1.0e3 +ts.tv_nsec*1.0e-6;
// 				if (curr_time_msec - ts_msec > timeout) {
// 					printf("[%s] Timeout! No msg received for %i msec.\n",self,timeout);
// 					break;
// 				}
// 			} else {
// 				printf ("[%s] could not get current time\n", self);
// 			}
//     		printf ("[%s] waiting for a reply. Could execute other code now.\n", self);
//     		zclock_sleep (1000);
//     	}




    destroy_component(&self);
    //  @end
    printf ("SHUTDOWN\n");
    return 0;
}


