# Example on how to add a new ARTVA measurement to the RSG.
# A single measurement is represented by a Node. 
# It has a relation to a GIS origin via Geo Pose (Transform with wgs84 tag, and latlon unit)  
# 
# The examples use the JSON API that sends a messages via
# ZMQ REQ-REP pattern to the SHERPA WM.  

import zmq
import random
import sys
import time
import json

# The port is defined by the system composition (sherpa_world_model.usc) file
# via the ``local_json_in_port`` variable. 
# e.g.
# local local_json_query_port = "22422"  
port = "22422" 

# Set up the ZMQ REQ-REP communication layer.
context = zmq.Context()

def sendMessageToSMW(message):
  socket = context.socket(zmq.REQ)
  socket.connect("tcp://localhost:%s" % port) # Currently he have to reconnect for every message.
  print("Sending update: %s " % (message))
  socket.send_string(message)
  result = socket.recv()
  print("Received result: %s " % (result))


# JSON message to CREATE an origin Node. (As child to the root node.)
newOriginMsg = {
  "@worldmodeltype": "RSGUpdate",
  "operation": "CREATE",
  "node": {
         
    "@graphtype": "Group",
    "id": "953cb0f0-e587-4880-affe-90001da1262d",
    "attributes": [
      {"key": "gis:origin", "value": "wgs84"},
      {"key": "comment", "value": "Reference frame for geo poses. Use this ID for Transform queries."},
    ],       

  },
  "parentId": "e379121f-06c6-4e21-ae9d-ae78ec1986a1",
}


# JSON message to CREATE a new Node. Note, that the parentId must
# exist beforehands, otherwise this operation does not succeed.
# in this case the "parentId": "3304e4a0-44d4-4fc8-8834-b0b03b418d5b"
# denotes the origin node. Though this is a rather arbitrary choice.
# The root node should be fine as well. As best practice a common "measurements" 
# Group is recommend.
newArtvaNodeMsg = {
  "@worldmodeltype": "RSGUpdate",
  "operation": "CREATE",
  "node": {
    "@graphtype": "Node",
    "id": "3e02fe9f-bc0a-4192-86d3-28dfff75f8c9", 
    "attributes": [
          {"key": "sherpa:artva_signal", "value": "77"},
          {"key": "sherpa:stamp", "value": "2016-05-22T11:48:05Z"},
    ],
  },
  "parentId": "953cb0f0-e587-4880-affe-90001da1262d",
}

# JSON message to CREATE a new Transform. Note that the parentId must
# exist beforehands, otherwise this operation does not succeed.
newTransformMsg = {
  "@worldmodeltype": "RSGUpdate",
  "operation": "CREATE",
  "node": {    
    "@graphtype": "Connection",
    "@semanticContext":"Transform",
    "id": "3304e4a0-44d4-4fc8-8834-b0b03b418d5b",
    "attributes": [
      {"key": "tf:type", "value": "wgs84"}
    ],
    "sourceIds": [
      "953cb0f0-e587-4880-affe-90001da1262d",
    ],
    "targetIds": [
      "3e02fe9f-bc0a-4192-86d3-28dfff75f8c9",
    ],
    "history" : [
      {
        "stamp": {
          "@stamptype": "TimeStampDate",
          "stamp": "2016-05-22T11:48:05Z",
        },
        "transform": {
          "type": "HomogeneousMatrix44",
            "matrix": [
              [1,0,0,44.153278],
              [0,1,0,12.241426],
              [0,0,1,2134],
              [0,0,0,1] 
            ],
            "unit": "latlon"
        }
      }
    ], 	    
  },
  "parentId": "953cb0f0-e587-4880-affe-90001da1262d",
}


# Send the messages.
sendMessageToSMW(json.dumps(newOriginMsg))
sendMessageToSMW(json.dumps(newArtvaNodeMsg))
sendMessageToSMW(json.dumps(newTransformMsg))


