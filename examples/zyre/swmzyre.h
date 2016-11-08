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

bool add_victim(double x, double y , double z, double utcTimeStampInMiliSec, char *author);

/* Handlers */

void handle_enter(component_t *self, zmsg_t *msg);

void handle_exit(component_t *self, zmsg_t *msg);

void handle_whisper (component_t *self, zmsg_t *msg);

void handle_shout(component_t *self, zmsg_t *msg, char **reply);

void handle_join (component_t *self, zmsg_t *msg);

void handle_evasive (component_t *self, zmsg_t *msg);



