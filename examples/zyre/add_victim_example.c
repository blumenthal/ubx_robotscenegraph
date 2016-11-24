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

	char agent_name[] = "fw0"; // or wasp1, operator0, ... donkey0, sherpa_box0. Please use the same as SWM_AGENT_NAME environment variable.

	/* Load configuration file for communication setup */
	char config_folder[255] = { SWM_ZYRE_CONFIG_DIR };
	char config_name[] = "swm_zyre_config.json";
	char config_file[512] = {0};
	snprintf(config_file, sizeof(config_file), "%s/%s", config_folder, config_name);

    if (argc == 2) { // override default config
    	snprintf(config_file, sizeof(config_file), "%s", argv[1]);
    }
    
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


	int i;
	struct timeval tp;

//	printf("###################### ORIGIN #########################\n");
//	char* originId = 0;
//	assert(get_gis_origin_id(self, &originId));
//	printf ("Origin ID = %s \n", originId);
//	free(originId);

	printf("###################### AGENT #########################\n");
	double matrix[16] = { 1, 0, 0, 0,
			               0, 1, 0, 0,
			               0, 0, 1, 0,
			               0, 0, 0, 1}; // y,x,z,1 remember this is column-major!
	matrix[12] = x;
	matrix[13] = y;
	matrix[14] = z;
	add_agent(self, matrix, utcTimeInMiliSec, agent_name); //TODO rotation/transform as 4x4 column-major matrix


	for (i = 0; i < 2; ++i) {
		printf("###################### VICTIM #########################\n");
		gettimeofday(&tp, NULL);
		utcTimeInMiliSec = tp.tv_sec * 1000 + tp.tv_usec / 1000; //get current timestamp in milliseconds
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
		double matrix[16] = { 1, 0, 0, 0,
				               0, 1, 0, 0,
				               0, 0, 1, 0,
				               0, 0, 0, 1}; // y,x,z,1 remember this is column-major!
		matrix[12] = x;
		matrix[13] = y;
		matrix[14] = z;
		assert(add_victim(self, matrix, utcTimeInMiliSec, agent_name));
	}

	for (i = 0; i < 2; ++i) {
		printf("###################### IMAGE #########################\n");
		gettimeofday(&tp, NULL);
		utcTimeInMiliSec = tp.tv_sec * 1000 + tp.tv_usec / 1000; //get current timestamp in milliseconds
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
		double matrix[16] = { 1, 0, 0, 0,
				               0, 1, 0, 0,
				               0, 0, 1, 0,
				               0, 0, 0, 1}; // y,x,z,1 remember this is column-major!
		matrix[12] = x;
		matrix[13] = y;
		matrix[14] = z;
		assert(add_image(self, matrix, utcTimeInMiliSec, agent_name, "/tmp/image001.jpg"));
	}

	for (i = 0; i < 30; ++i) {
			printf("######################  POSE  #########################\n");
			gettimeofday(&tp, NULL);
			utcTimeInMiliSec = tp.tv_sec * 1000 + tp.tv_usec / 1000; //get current timestamp in milliseconds
			double matrix[16] = { 1, 0, 0, 0,
					               0, 1, 0, 0,
					               0, 0, 1, 0,
					               0, 0, 0, 1}; // y,x,z,1 remember this is column-major!
			matrix[12] = x;
			matrix[13] = y;
			matrix[14] = z+i;
			update_pose(self, matrix, utcTimeInMiliSec+i, agent_name);
			usleep(100/*[ms]*/ * 1000);
	}

	printf("######################  GET POSITION  #########################\n");
	x = 0;
	y = 0;
	z = 0;
	gettimeofday(&tp, NULL);
	utcTimeInMiliSec = tp.tv_sec * 1000 + tp.tv_usec / 1000; //get current timestamp in milliseconds
	get_position(self, &x, &y, &z, utcTimeInMiliSec, agent_name);
	printf ("Latest position of hawk = (%f,%f,%f)\n", x,y,z);


	printf("######################  DONE  #########################\n");
    /* Clean up */
    destroy_component(&self);
    printf ("SHUTDOWN\n");

	return 0;

}


