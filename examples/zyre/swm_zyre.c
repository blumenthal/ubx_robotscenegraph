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


    char *msg;
    if (argc == 2) {

    	/* Query as specified by a file */
    	printf("\n");
    	printf("#########################################\n");
    	printf("[%s] Sending Generic Query\n",self->name);
    	printf("#########################################\n");
    	printf("\n");

    	msg = encode_json_message_from_file(self, argv[1]);

    } else {

		/* Query for root node */
		printf("\n");
		printf("#########################################\n");
		printf("[%s] Sending Query for RSG Root Node\n",self->name);
		printf("#########################################\n");
		printf("\n");

		json_t* query_params = NULL;
		msg = send_query(self,"GET_ROOT_NODE",query_params);
    }

    /* Send the message */
    shout_message(self, msg);

    /* Wait for a reply */
    char* reply = wait_for_reply(self);

    /* Print reply */
    printf("#########################################\n");
    printf("[%s] Got reply: %s \n", self->name, reply);

    /* Clean up */
    destroy_component(&self);
    printf ("SHUTDOWN\n");
    return 0;
}


