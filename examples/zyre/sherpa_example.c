/*
 * This example shows how to use the C "client" API.
 * It essentially wraps a subset of the JSON API,
 * relevant for a SHERPA mission.
 *
 * For communication zyre is used.
 * Please make ensure a correct zyre configuration file
 * is passed.
 */

#include <zyre.h>
#include <jansson.h>
#include <uuid/uuid.h>
#include <string.h>

#include "swmzyre.h"


/*
 * Example callback for incoming monitor messages
 */
void handle_monitor_msg(char *msg) {
	printf("handle_monitor_msg: msg =  %s\n", msg);
}

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

	/* Optional: register a callback for incoming monitor messages. */
	register_monitor_callback(self, &handle_monitor_msg);

	int i;
	struct timeval tp;

	printf("###################### CONNECTIVITY #########################\n");
	char *root_id = 0;
	/* Get the root node ID of the local SHWRPA World Model.
	 * This can be used the check connectivity to the SMW.
	 * Typically false means the local SWM cannot be reached. Is it actually started?
	 */
	assert(get_root_node_id(self, &root_id));
	free(root_id);


	printf("###################### AGENT #########################\n");
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
	assert(add_agent(self, matrix, utcTimeInMiliSec, agent_name));
	assert(add_agent(self, matrix, utcTimeInMiliSec, agent_name)); // twice is not a problem, sine it checks for existance

	/*
	 * Add new observations for potential victims
	 */
	for (i = 0; i < 2; ++i) {
		printf("###################### VICTIM #########################\n");
		gettimeofday(&tp, NULL);
		utcTimeInMiliSec = tp.tv_sec * 1000 + tp.tv_usec / 1000; //get current timestamp in milliseconds
		double matrix[16] = { 1, 0, 0, 0,
				               0, 1, 0, 0,
				               0, 0, 1, 0,
				               0, 0, 0, 1}; // y,x,z,1 remember this is column-major!
		matrix[12] = x;
		matrix[13] = y;
		matrix[14] = z;
		assert(add_victim(self, matrix, utcTimeInMiliSec, agent_name));
	}

	/*
	 * Add new image observations
	 */
	for (i = 0; i < 2; ++i) {
		printf("###################### IMAGE #########################\n");
		gettimeofday(&tp, NULL);
		utcTimeInMiliSec = tp.tv_sec * 1000 + tp.tv_usec / 1000; //get current timestamp in milliseconds
		double matrix[16] = { 1, 0, 0, 0,
				               0, 1, 0, 0,
				               0, 0, 1, 0,
				               0, 0, 0, 1}; // y,x,z,1 remember this is column-major!
		matrix[12] = x;
		matrix[13] = y;
		matrix[14] = z;
		assert(add_image(self, matrix, utcTimeInMiliSec, agent_name, "/tmp/image001.jpg"));
	}

	/*
	 * Add new ARTVA observations (Only relevant for the WASPS)
	 */
	for (i = 0; i < 2; ++i) {
		printf("###################### ARTVA #########################\n");
		gettimeofday(&tp, NULL);
//		utcTimeInMiliSec = tp.tv_sec * 1000 + tp.tv_usec / 1000; //get current timestamp in milliseconds
//		double matrix[16] = { 1, 0, 0, 0,
//				               0, 1, 0, 0,
//				               0, 0, 1, 0,
//				               0, 0, 0, 1}; // y,x,z,1 remember this is column-major!
//		matrix[12] = x;
//		matrix[13] = y;
//		matrix[14] = z;
//		assert(add_artva(self, matrix,  77, 12, 0, 0, utcTimeInMiliSec, agent_name));
		artva_measurement artva;
		artva.signal0 = 4104+i;
		artva.signal1 = 0;
		artva.signal2 = 0;
		artva.signal3 = 0;
		artva.angle0 = 49;
		artva.angle1 = 0;
		artva.angle2 = 0;
		artva.angle3 = 0;
		assert(add_artva_measurement(self, artva, agent_name));
	}

	/*
	 * Add new battery values. In fact it is stored in a single battery node and
	 * get updated after first creation.
	 */
	for (i = 0; i < 2; ++i) {
		printf("###################### BATTERY #########################\n");
		double voltage = 20 + i;
		assert(add_battery(self, voltage, "HIGH", utcTimeInMiliSec, agent_name));
	}

	/*
	 * Add new status values for the SHERPA box. In fact it is stored in a single node and
	 * get updated after first creation. Like the battery.
	 */
	for (i = 0; i < 5; ++i) {
		printf("###################### SHERPA BOX STATUS #########################\n");
		sbox_status status;
		status.commandStep = 0;
		status.completed = i;
		status.executeId = 0;
		status.idle = 0;
		status.linActuatorPosition = 0,
		status.waspDockLeft = false;
		status.waspDockRight = true;
		status.waspLockedLeft = false;
		status.waspLockedRight = false;
		assert(add_sherpa_box_status(self, status, agent_name));
	}

	/*
	 * Add new status values for the WASP similar to the SHERPA Box.
	 */
	for (i = 0; i < 2; ++i) {
		printf("###################### SHERPA WASP FLIGHT STATUS #########################\n");
		wasp_flight_status status;
		status.flight_state = "ON_GROUND_ARMED";
		assert(add_wasp_flight_status(self, status, agent_name));
	}

	/*
	 * Add new status values for the WASP similar to the SHERPA Box.
	 */
	for (i = 0; i < 2; ++i) {
		printf("###################### SHERPA WASP DOCK STATUS #########################\n");
		wasp_dock_status status;
		status.wasp_on_box = "NOT";
		assert(add_wasp_dock_status(self, status, agent_name));
	}

	/*
	 * Update pose of this agent
	 */
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

	/*
	 * Get current position
	 */
	printf("######################  GET POSITION  #########################\n");
	x = 0;
	y = 0;
	z = 0;
	gettimeofday(&tp, NULL);
	utcTimeInMiliSec = tp.tv_sec * 1000 + tp.tv_usec / 1000; //get current timestamp in milliseconds
	get_position(self, &x, &y, &z, utcTimeInMiliSec, agent_name);
	printf ("Latest position of agent = (%f,%f,%f)\n", x,y,z);

	/*
	 * Get ID of mediator
	 */
	printf("######################  GET MEDIATOR ID  #########################\n");
	char* mediator_id = NULL;
	assert(get_mediator_id(self, &mediator_id));
	printf ("ID of mediator = %s\n", mediator_id);
	free(mediator_id);

	/*
	 * Get status of sherpa box
	 */
	printf("######################  GET SHERPA BOX STATUS  #########################\n");
	sbox_status sbox_status;
	assert(get_sherpa_box_status(self, &sbox_status, agent_name));
	printf("SBOX: %i, %i, %i, %i, %i,   %i, %i, %i, %i \n", sbox_status.idle, sbox_status.completed, sbox_status.executeId,
			sbox_status.commandStep, sbox_status.linActuatorPosition, sbox_status.waspDockLeft, sbox_status.waspDockRight,
			sbox_status.waspLockedLeft, sbox_status.waspLockedRight);

	assert(get_sherpa_box_status(self, &sbox_status, 0)); // search globally with out any agent name. Since there is only one SHERPA box...
	printf("SBOX: %i, %i, %i, %i, %i,   %i, %i, %i, %i \n", sbox_status.idle, sbox_status.completed, sbox_status.executeId,
			sbox_status.commandStep, sbox_status.linActuatorPosition, sbox_status.waspDockLeft, sbox_status.waspDockRight,
			sbox_status.waspLockedLeft, sbox_status.waspLockedRight);


	/*
	 * Get flight/docking status
	 */
	printf("######################  GET WASP STATUS  #########################\n");
	wasp_flight_status flight_status;
	assert(get_wasp_flight_status(self, &flight_status,agent_name));
	printf("WASP flight status = %s\n", flight_status.flight_state);


	wasp_dock_status dock_status;
	assert(get_wasp_dock_status(self, &dock_status,agent_name));
	printf("WASP dock status = %s\n", dock_status.wasp_on_box);

	/*
	 * Digital Elevation Map
	 */
	printf("######################  DEM  #########################\n");
	char map_file_name [512] = {0};
	char project_path[512] = { SWM_ZYRE_CONFIG_DIR };
	snprintf(map_file_name, sizeof(map_file_name), "%s%s", project_path, "/../maps/dem/davos.tif");

	/* A map has to be loaded first, then we get queries. */
	assert(load_dem(self, &map_file_name));

	/* Get elevation a a specific location. Queries beyond the map will return false  */
	double elevation = 0;
	assert(!get_elevataion_at(self, &elevation, 0, 0)); // this should fail
	printf("DEM: elevation = %lf \n", elevation);

	assert(get_elevataion_at(self, &elevation, 9.849468, 46.812785)); // this should work for the davos map
	printf("DEM: elevation = %lf \n", elevation);


	printf("######################  DONE  #########################\n");
    /* Clean up */
    destroy_component(&self);
    printf ("SHUTDOWN\n");

	return 0;

}


