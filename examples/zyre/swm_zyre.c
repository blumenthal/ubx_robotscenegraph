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

	
	

    destroy_component(&self);
    //  @end
    printf ("SHUTDOWN\n");
    return 0;
}


