/**
 * Helper library for sending RSG-JSON messages to the SWM via Zyre.
 * It is intended to be reused by the other componentnes.
 *
 *  Cf. swm_zyre.c as an example
 */

#include <zyre.h>
#include <jansson.h>
#include <uuid/uuid.h>
#include <string.h>

// (Internal) Helper structs

typedef struct _json_msg_t {
    char *metamodel;
    char *model;
    char *type;
    char *payload;
} json_msg_t;

typedef struct _query_t {
        const char *uid;
        const char *requester;
        json_msg_t *msg;
        zactor_t *loop;
} query_t;

/// Callback for potential incoming monitor messages.
typedef void (*monitor_callback_t)(char *);

typedef struct _component_t {
	const char *name;
	const char *localgroup;
	char *RSG_parent;
	zyre_t *local;
	json_t *config;
	zactor_t *communication_actor;
	zlist_t *query_list;
	int timeout;
	int no_of_updates;
	int no_of_queries;
	int no_of_fcn_block_calls;
	int alive;
	monitor_callback_t monitor;
} component_t;


typedef struct _sbox_status_t {
	int idle;
	int completed;
	int executeId;   //(changeBattery=31, lock wasp=1, moveBat_Clamps=2, moveLinAct=5,moveRevolver=4, zeroRevolver=3)
	int commandStep; // micro steps for debugging only
	int linActuatorPosition;
	bool waspDockLeft;
	bool waspDockRight;
	bool waspLockedLeft;
	bool waspLockedRight;
} sbox_status;


// flight state:
/*#define ON_GROUND_NO_HOME 10
#define SETTING_HOME 20
#define ON_GROUND_DISARMED 30
#define ARMING 40
#define DISARMING 45
#define ON_GROUND_ARMED 50
#define PERFORMING_TAKEOFF 70
#define IN_FLIGHT 80
#define GRID 90
#define PERFORMING_GO_TO 100
#define PERFORMING_LANDING 120
#define LEASHING 140
#define PAUSED 150
#define MANUAL_FLIGHT 1000*/
typedef struct _wasp_flight_status_t {
	char* flight_state;
} wasp_flight_status;

//wasp_on_box:
//“NOT”, “LEFT”, “RIGHT”​
typedef struct _wasp_dock_status_t {
	char* wasp_on_box;
} wasp_dock_status;

typedef struct _artva_measurement_t {
	int signal0;
	int signal1;
	int signal2;
	int signal3;
	int angle0;
	int angle1;
	int angle2;
	int angle3;
} artva_measurement;

// Internal helper methods

void query_destroy (query_t **self_p);

void destroy_message(json_msg_t *msg);

void destroy_component (component_t **self_p);

/**
 * Register a monitor callback for _all_ incoming messages of type RSGMontitor.
 * @param self The communication component that "listens" to incoming messages.
 * @param monitor Call back that gets called. The RSG message will be passed as parameter.
 *                To unregister, set the value to 0. This is also the default.
 */
void register_monitor_callback(component_t* self, monitor_callback_t monitor);

query_t * query_new (const char *uid, const char *requester, json_msg_t *msg, zactor_t *loop);

component_t* new_component(json_t *config);

json_t * load_config_file(char* file);

int decode_json(char* message, json_msg_t *result);

/**
 * Send a messages as payload. The method will try to extract a queryId.
 * @self Communication component
 * @message_file file a RAW RSG-JSON message. Missing quiryIds are filled in automatically
 */
char* encode_json_message_from_file(component_t* self, char* message_file);

char* encode_json_message_from_string(component_t* self, char* message);

char* encode_json_message(component_t* self, json_t* message);

char* wait_for_reply(component_t* self, char *msg, int timeout);

int shout_message(component_t* self, char* message);

/* Convenience functions */
char* send_query(component_t* self, char* query_type, json_t* query_params);

char* send_update(component_t* self, char* operation, json_t* update_params);

/*
 * Convenience functions for a SHERPA mission
 *
 * Conventions on used data types can be found here:
 * https://github.com/blumenthal/sherpa_world_model_knowrob_bridge/blob/master/doc/codebook.md
 */

/**
 * Add an agent (SHARPA animal) to the world model.
 * Can be safely called multiple times, since it checks if it exists already.
 * In case it exists it will (just) update the pose.
 * @param[in] self Handle to the communication component.
 * @param[in] transform_matrix 4x4 Homogeneous matrix represents as column-major array. (Like e.g. Eigen). @see update_pose for further details.
 * @param[in] utc_time_stamp_in_mili_sec UTC time stamp since epoch (1970) in [ms].
 * @param[in] agent_name Name of the agent. e.g. "fw0", "operator0", or "wasp2", ...
 * @return True if agent was successfully added or existed already. In case of an error false.
 */
bool add_agent(component_t *self, double* transform_matrix, double utc_time_stamp_in_mili_sec, char *agent_name);

/**
 * An an observation of a (potential) victim.
 * Every observation is represented as a single node.
 * @param[in] self Handle to the communication component.
 * @param[in] transform_matrix 4x4 Homogeneous matrix represents as column-major array. (Like e.g. Eigen). @see update_pose for further details.
 * @param[in] utc_time_stamp_in_mili_sec UTC time stamp since epoch (1970) in [ms].
 * @param[in] author Agent that created the observation. Same as agent_name in other methods. e.g. "fw0", "operator0", or "wasp2", ...
 * @return True if victim was sucesfully added, otherwise false.
 */
bool add_victim(component_t *self, double* transform_matrix, double utc_time_stamp_in_mili_sec, char* author);

/**
 * Add an observation image.
 * Every observation is represented as a single node.
 * @param[in] self Handle to the communication component.
 * @param[in] transform_matrix 4x4 Homogeneous matrix represents as column-major array. (Like e.g. Eigen). @see update_pose for further details.
 * @param[in] utc_time_stamp_in_mili_sec UTC time stamp since epoch (1970) in [ms].
 * @param[in] author Agent that created the observation. Same as agent_name in other methods. e.g. "fw0", "operator0", or "wasp2", ...
 * @param[in] file_name File name of the locally stored image. Use absolute paths. e.g.  /tmp/sherpa/image0001.jpg
 * @return True if image was sucesfully added, otherwise false.
 */
bool add_image(component_t *self, double* transform_matrix, double utc_time_stamp_in_mili_sec, char* author, char* file_name);

/**
 * Add an ARTVA observation
 * A single observation stores 4 different channels at the same time.
 * @param[in] self Handle to the communication component.
 * @param[in] transform_matrix 4x4 Homogeneous matrix represents as column-major array. (Like e.g. Eigen). @see update_pose for further details.
 * @param[in] utc_time_stamp_in_mili_sec UTC time stamp since epoch (1970) in [ms].
 * @param[in] author Agent that created the observation. Same as agent_name in other methods. e.g. "fw0", "operator0", or "wasp2", ...
 * @param[in] artva0 ARTVAL signal 0
 * @param[in] artva1 ARTVAL signal 1
 * @param[in] artva2 ARTVAL signal 2
 * @param[in] artva3 ARTVAL signal 3
 * @return True if ARTVA observation was sucesfully added, otherwise false.
 */
//bool add_artva(component_t *self, double* transform_matrix, double artva0, double artva1, double artva2, double artva3,
//		double utc_time_stamp_in_mili_sec, char* author);

bool add_artva_measurement(component_t *self, artva_measurement measurement, char* author);

bool add_battery(component_t *self, double battery_voltage, char* battery_status,  double utc_time_stamp_in_mili_sec, char* author);

bool add_sherpa_box_status(component_t *self, sbox_status status, char* author);

bool add_wasp_flight_status(component_t *self, wasp_flight_status status, char* author);

bool get_wasp_flight_status(component_t *self, wasp_flight_status* status, char* agent_name);

bool add_wasp_dock_status(component_t *self, wasp_dock_status status, char* author);

bool get_wasp_dock_status(component_t *self, wasp_dock_status* status, char* agent_name);

/**
 * Add am area with a custom name. It is defined by a closed polygon on latitude and longitude coordinates.
 * @param[in] self Communication component.
 * @param polygon_coordinates Array with coordinates on form of:
 *  [lat0,lon0 ,lat1,lon1, lat2, lon2, ... ]
 * @param[in] num_coordinates Number of coordinates in above array. E.g. for 3 coordinates len = 3 Note, the array actually has 6 elements.
 * @paramV area_name Name to identify the area. E.g. "mission_area0"
 * @return True on success.
 */
bool add_area(component_t *self, double *polygon_coordinates, int num_coordinates, char* area_name);

/**
 * Load a DEM.
 * @param[in] self Communication component
 * @param[in] map_file_name Map in geotiff format.
 * @return True on sccucess
 */
bool load_dem(component_t *self, char* map_file_name);

/**
 * Get elevation at a location. load_dem() has to be called first!
 * @param[in] self Communication component
 * @param[out] elevation Elevation in [m] Will be set -1.0 in case of an error.
 * @param[in] latitude Coordinate
 * @param[in] longitude Coordinate
 * @return True on success. Will be false for invalid elevation data or queries beyond the map dimensions.
 */
bool get_elevataion_at(component_t *self, double* elevation, double latitude, double longitude);

/**
 * Get elevation at a location. load_dem() and add_area() have to be called first!
 * @param[in] self Communication component
 * @param min_elevation Min elevation in [m] Will be set -1.0 in case of an error.
 * @param max_elevation Max elevation in [m] Will be set -1.0 in case of an error.
 * @param area_name The name of an are. Same as used in add_area.
 * @return True on success. Will be false for a non existing area.
 */
bool get_min_max_elevation_in_area(component_t *self, double* min_elevation, double* max_elevation, char* area_name);

/**
 * Update pose of agent.
 * The agent must exist before hands.
 * Note, this is a more light weight version off add_agent() since it performs less checks.
 * @param[in] self Handle to the communication component.
 * @param[in] transform_matrix 4x4 Homogeneous matrix represents as column-major array. (Like e.g. Eigen).
 *
 * column-major layout:
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
 *
 * @param[in] utc_time_stamp_in_mili_sec UTC time stamp since epoch (1970) in [ms].
 * @param[in] author Agent that created the observation. Same as agent_name in other methods. e.g. "fw0", "operator0", or "wasp2", ...
 * @return True if pose was sucesfully updated, otherwise false. The latter is the case e.g. when the agent was not created beforehand.
 */
bool update_pose(component_t *self, double* transform_matrix, double utc_time_stamp_in_mili_sec, char *agent_name);


/**
 * Get the position x,y,z (lat,lon,att) of an agent.
 *
 * @param self Handle to the communication component.
 * @param xOut
 * @param yOut
 * @param zOut
 * @param utc_time_stamp_in_mili_sec UTC time stamp since epoch (1970) in [ms].
 *        Used to determine at which point in time the position shall be return.
 *        Use a current time stamp to retrieve the latest postition.
 * @param agent_name Name of the agent. e.g. "fw0", "operator0", or "wasp2", ...
 * @return
 */
bool get_position(component_t *self, double* xOut, double* yOut, double* zOut, double utc_time_stamp_in_mili_sec, char *agent_name);

bool get_pose(component_t *self, double* transform_matrix, double utc_time_stamp_in_mili_sec, char *agent_name);

bool get_sherpa_box_status(component_t *self, sbox_status* status, char* agent_name);


/**
 * Get the root node ID of the local SHWRPA World Model.
 * This can be used the check connectivity the the SMW.
 * @param [in]self Handle to the communication component.
 * @param[out] root_id Resulting root node ID or NULL. Owned by caller, so it is has to be freed afterwards.
 * @return True if root node was sucesfully found, otherwise false. Typically false means the local SWM cannot be reached. Is it actually started?
 */
bool get_root_node_id(component_t *self, char** root_id);

/**
 * Get the ID of the origin node, based on an attribute look up.
 * @param [in]self Handle to the communication component.
 * @param[out] origin_id Resulting ID or NULL. Owned by caller, so it is has to be freed afterwards.
 * @return True if origin was sucesfully found, otherwise false.
 */
bool get_gis_origin_id(component_t *self, char** origin_id);

/**
 * Get the ID of the observations group, based on an attribute look up.
 * @param [in]self Handle to the communication component.
 * @param[out] observations_id Resulting ID or NULL. Owned by caller, so it is has to be freed afterwards.
 * @return True if origin was sucesfully found, otherwise false.
 */
bool get_observations_group_id(component_t *self, char** observations_id);

/**
 * Analog to get_observations_group_id
 */
bool get_status_group_id(component_t *self, char** status_id);


/**
 * Get a node by a single specific attribute.
 * @param[in] self Handle to the communication component.
 * @param[out] node_id Resulting ID or NULL. Owned by caller, so it is has to be freed afterwards.
 * @param[in] key Attribute key.
 * @param[in] value Attribute value.
 * @return True if node was sucesfully found, otherwise false.
 */
bool get_node_by_attribute(component_t *self, char** node_id, const char* key, const char* value);

bool get_node_by_attribute_in_subgrapgh(component_t *self, char** node_id, const char* key, const char* value,  const char* subgraph_id);

/**
 * Add gepose between the origin and an existing node as defined by node_id.
 * @param[in] self Handle to the communication component.
 * @param[in] node_id ID that specifies to whom the geopose should point.
 * @param[out] new_geopose_id Resulting ID or NULL. Owned by caller, so it is has to be freed afterwards.
 * @param[in] transform_matrix 4x4 Homogeneous matrix represents as column-major array. (Like e.g. Eigen)
 *
 * column-major layout:
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
 *
 * @param key Optional attribute key. Ignored on NULL
 * @param value Optional attribute value. Ignored on NULL
 * @return Resulting ID or NULL. Owned by caller, so it is has to be freed afterwards.
 */
bool add_geopose_to_node(component_t *self, char* node_id, char** new_geopose_id, double* transform_matrix, double utc_time_stamp_in_mili_sec, const char* key, const char* value, int max_cache_duration_in_sec);

/**
 * Get the UUID of the Mediator component by using "query_mediator_uuid" query type.
 * @param[in] self Handle to the communication component.
 * @param[out] mediator_id Resulting ID or NULL. Owned by caller, so it is has to be freed afterwards.
 * @return True if ID was successfully found, otherwise false. Typically the case when no Mediator is used.
 */
bool get_mediator_id(component_t *self, char** mediator_id);

/**
 * Get a file via the Mediator from a remote Mediator
 * @param self  Communication component
 * @param uri URI in form of <Mediator UUID>:<fileName> e.g. 2BC298A07C76C6D6A46337516BEEDEBB:/tmp/img045.jpg
 * @param target_file_name File name on local file sytem e.g /tmp/img045.jpg
 * @return
 */
bool get_file_by_uri(component_t *self, char* uri, char* target_file_name);


/* Internal handlers */
void handle_enter(component_t *self, zmsg_t *msg);

void handle_exit(component_t *self, zmsg_t *msg);

void handle_whisper (component_t *self, zmsg_t *msg);

void handle_shout(component_t *self, zmsg_t *msg, char **rep);

void handle_join (component_t *self, zmsg_t *msg);

void handle_evasive (component_t *self, zmsg_t *msg);



