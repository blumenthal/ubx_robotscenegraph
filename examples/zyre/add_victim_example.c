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

	/* Input variables */
	double x = 979875;
	double y = 48704;
	double z = 405;
	double utcTimeInMiliSec = 0.0;

	add_victim(x,y,z,utcTimeInMiliSec, "hawk", config_file);
	add_agent(x,y,z,utcTimeInMiliSec, "hawk", config_file); //TODO rotation/transform as 4x4 column-major matrix
	update_pose(x,y,z+1,utcTimeInMiliSec+1, "hawk", config_file);
	return 0;

}


