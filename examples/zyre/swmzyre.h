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

typedef struct _component_t {
	const char *name;
	const char *localgroup;
	char *RSG_parent;
	zyre_t *local;
	json_t *config;
	zpoller_t *poller;
	zlist_t *query_list;
	int timeout;
	int no_of_updates;
	int no_of_queries;
	int no_of_fcn_block_calls;
	int alive;
} component_t;

void query_destroy (query_t **self_p);

void destroy_message(json_msg_t *msg);

void destroy_component (component_t **self_p);

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

char* wait_for_reply(component_t* self);

int shout_message(component_t* self, char* message);

/* Convenience functions */
char* send_query(component_t* self, char* query_type, json_t* query_params);

char* send_update(component_t* self, char* operation, json_t* update_params);

/* Convenience functions for a SHERPA mission*/
bool add_victim(component_t *self, double* transform_matrix, double utcTimeStampInMiliSec, char* author);

bool add_image(component_t *self, double* transform_matrix, double utcTimeStampInMiliSec, char* author, char* file_name);

bool add_agent(component_t *self, double* transform_matrix, double utcTimeStampInMiliSec, char *agentName);

bool update_pose(component_t *self, double* transform_matrix, double utcTimeStampInMiliSec, char *agentName);

bool get_position(component_t *self, double* xOut, double* yOut, double* zOut, double utcTimeStampInMiliSec, char *agentName);

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
 * Get a node by a single specific attribute.
 * @param[in] self Handle to the communication component.
 * @param[out] node_id Resulting ID or NULL. Owned by caller, so it is has to be freed afterwards.
 * @param[in] key Attribute key.
 * @param[in] value Attribute value.
 * @return c
 */
bool get_node_by_attribute(component_t *self, char** node_id, const char* key, const char* value);

/**
 * Add gepose between the origin and an existing node as defined by node_id.
 * @param[in] self Handle to the communication component.
 * @param[in] node_id ID that specifies to whom the geopose should point.
 * @param[out] new_geopose_id Resulting ID or NULL. Owned by caller, so it is has to be freed afterwards.
 * @param transform_matrix 4x4 Homogeneous matrix represents as column-major array. (Like e.g. Eigen)
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
bool add_geopose_to_node(component_t *self, char* node_id, char** new_geopose_id, double* transform_matrix, double utcTimeStampInMiliSec, const char* key, const char* value);

/* Handlers */

void handle_enter(component_t *self, zmsg_t *msg);

void handle_exit(component_t *self, zmsg_t *msg);

void handle_whisper (component_t *self, zmsg_t *msg);

void handle_shout(component_t *self, zmsg_t *msg, char **reply);

void handle_join (component_t *self, zmsg_t *msg);

void handle_evasive (component_t *self, zmsg_t *msg);



