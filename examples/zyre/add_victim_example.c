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

    json_t * config = load_config_file(config_file);//"swm_zyre_config.json");
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

	/* Input variables */
	double x = 979875;
	double y = 48704;
	double z = 405;
	double utcTimeInMiliSec = 0.0;

	printf("###################### VICTIM #########################\n");
	add_victim(self, x,y,z,utcTimeInMiliSec, "hawk");
	printf("###################### AGENT #########################\n");
	add_agent(self,  x,y,z,utcTimeInMiliSec, "hawk"); //TODO rotation/transform as 4x4 column-major matrix


	int i;
	for (i = 0; i < 10; ++i) {
			printf("######################  POSE  #########################\n");
			struct timeval tp;
			gettimeofday(&tp, NULL);
			utcTimeInMiliSec = tp.tv_sec * 1000 + tp.tv_usec / 1000; //get current timestamp in milliseconds
			update_pose(self, x,y,z+i,utcTimeInMiliSec+i, "hawk");
	}

	printf("######################  DONE  #########################\n");
    /* Clean up */
    destroy_component(&self);
    printf ("SHUTDOWN\n");

	return 0;

}


