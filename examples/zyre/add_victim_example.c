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

	/* Input variables */
	double x = 979875;
	double y = 48704;
	double z = 405;
	double utcTimeInMiliSec = 0.0;

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

    /* Get observationGroupId */
//    getObservationGroup = {
//      "@worldmodeltype": "RSGQuery",
//      "query": "GET_NODES",
//      "attributes": [
//          {"key": "name", "value": "observations"},
//      ]
//    }
	json_t *getObservationGroupMsg = json_object();
	json_object_set(getObservationGroupMsg, "@worldmodeltype", json_string("RSGQuery"));
	json_object_set(getObservationGroupMsg, "query", json_string("GET_NODES"));
	json_t *queryAttribute = json_object();
	json_object_set(queryAttribute, "key", json_string("name"));
	json_object_set(queryAttribute, "value", json_string("observations"));
	json_t *attributes = json_array();
	json_array_append(attributes, queryAttribute);
	json_object_set(attributes, "attributes", queryAttribute);
	json_object_set(getObservationGroupMsg, "attributes", attributes);

	/* Send message and wait for reply */
    msg = encode_json_message(self, getObservationGroupMsg);
    shout_message(self, msg);
    char* reply = wait_for_reply(self);
    printf("#########################################\n");
    printf("[%s] Got reply: %s \n", self->name, reply);

    json_decref(attributes);
    json_decref(queryAttribute);
    json_decref(getObservationGroupMsg);

    json_error_t error;
    json_t *observationGroupIdReply = json_loads(reply, 0, &error);
//    printf("[%s] message : %s\n", self->name , json_dumps(observationGroupIdReply, JSON_ENCODE_ANY));
    char * observationGroupId;
    json_t* observationGroupIdAsJSON = 0;
    json_t* array = json_object_get(observationGroupIdReply, "ids");
    if (array) {
//    	printf("[%s] result array found: %s \n", self->name);
    	if( json_array_size(array) > 0 ) {
    		observationGroupIdAsJSON = json_array_get(array, 0);
        	observationGroupId = strdup( json_dumps(json_array_get(array, 0), JSON_ENCODE_ANY) );
        	printf("[%s] ID is: %s \n", self->name, observationGroupId);
    	}
    }

    if(!observationGroupIdAsJSON) {
    	printf("[%s] [ERROR] Cannot get observation group Id \n", self->name);
    	return -1;
    }

    /*
     * The actual "observation" node. Here for a victim.
     */

    //    currentTimeStamp = datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S')
    //    imageNodeId = str(uuid.uuid4())
    //    newImageNodeMsg = {
    //      "@worldmodeltype": "RSGUpdate",
    //      "operation": "CREATE",
    //      "node": {
    //        "@graphtype": "Node",
    //        "id": imageNodeId,
    //        "attributes": [
    //              {"key": "sherpa:observation_type", "value": "image"},
    //              {"key": "sherpa:uri", "value": URI},
    //              {"key": "sherpa:stamp", "value": currentTimeStamp},
    //              {"key": "sherpa:author", "value": author},
    //        ],
    //      },
    //      "parentId": observationGroupId,
    //    }

    // top level message
	json_t *newImageNodeMsg = json_object();
	json_object_set(newImageNodeMsg, "@worldmodeltype", json_string("RSGUpdate"));
	json_object_set(newImageNodeMsg, "operation", json_string("CREATE"));
	json_object_set(newImageNodeMsg, "parentId", observationGroupIdAsJSON);
	json_t *newImageNode = json_object();
	json_object_set(newImageNode, "@graphtype", json_string("Node"));
	zuuid_t *uuid = zuuid_new ();
	json_object_set(newImageNode, "id", json_string(zuuid_str_canonical(uuid)));

	// attributes
	attributes = json_array();
	json_t *attribute1 = json_object();
	json_object_set(attribute1, "key", json_string("sherpa:observation_type"));
	json_object_set(attribute1, "value", json_string("victim"));
	json_array_append(attributes, attribute1);
//	json_t *attribute2 = json_object();
//	json_object_set(attribute2, "key", json_string("sherpa:uri"));
//	json_object_set(attribute2, "value", json_string("TODO"));
//	json_array_append(attributes, attribute2);
	json_t *attribute3 = json_object();
	json_object_set(attribute3, "key", json_string("sherpa:stamp"));
	json_object_set(attribute3, "value", json_real(utcTimeInMiliSec));
	json_array_append(attributes, attribute3);
	json_t *attribute4 = json_object();
	json_object_set(attribute4, "key", json_string("sherpa:author"));
	json_object_set(attribute4, "value", json_string("hawk"));
	json_array_append(attributes, attribute4);


	json_object_set(newImageNode, "attributes", attributes);
	json_object_set(newImageNodeMsg, "node", newImageNode);

	/* CReate message*/
    msg = encode_json_message(self, newImageNodeMsg);
    /* Send the message */
    shout_message(self, msg);
    /* Wait for a reply */
     reply = wait_for_reply(self);
    /* Print reply */
    printf("#########################################\n");
    printf("[%s] Got reply: %s \n", self->name, reply);

	// Clean up
    json_decref(attributes);
    json_decref(attribute1);
//    json_decref(attribute2);
    json_decref(attribute3);
    json_decref(attribute4);
    json_decref(newImageNodeMsg);
    json_decref(newImageNode);
    json_decref(uuid);


    /*
     * Get the "origin" node. It is relevant to specify a new pose.
     */
	json_t *getOriginMsg = json_object();
	json_object_set(getOriginMsg, "@worldmodeltype", json_string("RSGQuery"));
	json_object_set(getOriginMsg, "query", json_string("GET_NODES"));
	json_t * originAttribute = json_object();
	json_object_set(originAttribute, "key", json_string("gis:origin"));
	json_object_set(originAttribute, "value", json_string("wgs84"));
	attributes = json_array();
	json_array_append(attributes, originAttribute);
	json_object_set(getOriginMsg, "attributes", attributes);

	/* Send message and wait for reply */
    msg = encode_json_message(self, getOriginMsg);
    shout_message(self, msg);
    reply = wait_for_reply(self); // TODO free older reply
    printf("#########################################\n");
    printf("[%s] Got reply: %s \n", self->name, reply);

    /* Parse reply */
    json_t *originIdReply = json_loads(reply, 0, &error);
    json_t* originIdAsJSON = 0;
    array = json_object_get(originIdReply, "ids");
    if (array) {
    	printf("[%s] result array found: %s \n", self->name);
    	if( json_array_size(array) > 0 ) {
    		originIdAsJSON = json_array_get(array, 0);
        	printf("[%s] ID is: %s \n", self->name, json_dumps(originIdAsJSON, JSON_ENCODE_ANY));
    	}
    }

    /* Clean up */
    json_decref(attributes);
    json_decref(originAttribute);
    json_decref(getOriginMsg);

    /*
     * Pose
     */
//    newTransformMsg = {
//      "@worldmodeltype": "RSGUpdate",
//      "operation": "CREATE",
//      "node": {
//        "@graphtype": "Connection",
//        "@semanticContext":"Transform",
//        "id": tfId,
//        "attributes": [
//          {"key": "tf:type", "value": "wgs84"}
//        ],
//        "sourceIds": [
//          originId,
//        ],
//        "targetIds": [
//          imageNodeId,
//        ],
//        "history" : [
//          {
//            "stamp": {
//              "@stamptype": "TimeStampDate",
//              "stamp": currentTimeStamp,
//            },
//            "transform": {
//              "type": "HomogeneousMatrix44",
//                "matrix": [
//                  [1,0,0,x],
//                  [0,1,0,y],
//                  [0,0,1,z],
//                  [0,0,0,1]
//                ],
//                "unit": "latlon"
//            }
//          }
//        ],
//      },
//      "parentId": originId,
//    }

    // top level message
	json_t *newTfNodeMsg = json_object();
	json_object_set(newTfNodeMsg, "@worldmodeltype", json_string("RSGUpdate"));
	json_object_set(newTfNodeMsg, "operation", json_string("CREATE"));
	json_object_set(newTfNodeMsg, "parentId", observationGroupIdAsJSON);
	json_t *newTfConnection = json_object();
	json_object_set(newTfConnection, "@graphtype", json_string("Connection"));
	json_object_set(newTfConnection, "@semanticContext", json_string("Transform"));
	zuuid_t *poseUuid = zuuid_new ();
	json_object_set(newTfConnection, "id", json_string(zuuid_str_canonical(poseUuid)));
	// Attributes
	json_t *poseAttribute = json_object();
	json_object_set(poseAttribute, "key", json_string("name"));
	json_object_set(poseAttribute, "value", json_string("observations"));
	json_t *poseAttributes = json_array();
	json_array_append(poseAttributes, poseAttribute);
	json_object_set(newTfConnection, "attributes", poseAttributes);
	// sourceIds
	json_t *sourceIds = json_array();
	json_array_append(sourceIds, originIdAsJSON); // ID of origin node
	json_object_set(newTfConnection, "sourceIds", sourceIds);
	// sourceIds
	json_t *targetIds = json_array();
	json_array_append(targetIds, json_string(zuuid_str_canonical(uuid))); // ID of node that we just have created before
	json_object_set(newTfConnection, "targetIds", targetIds);

	// history
	json_t *history = json_array();
	json_t *stampedPose = json_object();
	json_t *stamp = json_object();
	json_t *pose = json_object();

	// stamp
	json_object_set(stamp, "@stamptype", json_string("TimeStampUTCms"));
//    struct timespec curr_time = {0,0};
//    clock_gettime(CLOCK_MONOTONIC,&curr_time); // TODO check for utc time; e.g. from the ROS header. To be tested!
//    double ts_msec = curr_time.tv_sec*1.0e3 +curr_time.tv_nsec*1.0e-6;
	json_object_set(stamp, "stamp", json_real(utcTimeInMiliSec));

	//pose
	json_object_set(pose, "type", json_string("HomogeneousMatrix44"));
	json_object_set(pose, "unit", json_string("latlon"));
	json_t *matrix = json_array();
	json_t *row0 = json_array();
	json_array_append(row0, json_real(1));
	json_array_append(row0, json_real(0));
	json_array_append(row0, json_real(0));
	json_array_append(row0, json_real(x));
	json_t *row1 = json_array();
	json_array_append(row1, json_real(0));
	json_array_append(row1, json_real(1));
	json_array_append(row1, json_real(0));
	json_array_append(row1, json_real(y));
	json_t *row2 = json_array();
	json_array_append(row2, json_real(0));
	json_array_append(row2, json_real(0));
	json_array_append(row2, json_real(1));
	json_array_append(row2, json_real(z));
	json_t *row3 = json_array();
	json_array_append(row3, json_real(0));
	json_array_append(row3, json_real(0));
	json_array_append(row3, json_real(0));
	json_array_append(row3, json_real(1));
	json_array_append(matrix, row0);
	json_array_append(matrix, row1);
	json_array_append(matrix, row2);
	json_array_append(matrix, row3);

	json_object_set(pose, "matrix", matrix);

	json_object_set(stampedPose, "stamp", stamp);
	json_object_set(stampedPose, "transform", pose);
	json_array_append(history, stampedPose);
	json_object_set(newTfConnection, "history", history);
	json_object_set(newTfNodeMsg, "node", newTfConnection);

	/* Send message and wait for reply */
    msg = encode_json_message(self, newTfNodeMsg);
    shout_message(self, msg);
    reply = wait_for_reply(self); // TODO free older reply
    printf("#########################################\n");
    printf("[%s] Got reply for pose: %s \n", self->name, reply);

    /* Clean up */
    destroy_component(&self);
    printf ("SHUTDOWN\n");
    return 0;
}


