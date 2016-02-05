# Example on how to add a new object with an associated pose.
# 
# The examples use the JSON API that sends a messages via
# ZMQ to the SHERPA WM.  

import zmq
import random
import sys
import time
import json

# The port is defined by the system composition (sherpa_world_model.usc) file
# via the ``local_json_in_port`` variable. 
# e.g.
# local local_json_in_port = "12911"  
port = "12911" 
if len(sys.argv) > 1:
    port =  sys.argv[1]
    int(port)

# Set up the ZMQ PUB-SUB communication layer.
context = zmq.Context()
socket = context.socket(zmq.PUB)
socket.bind("tcp://*:%s" % port)

# JSON message to CREATE a an origin Node. (As child to thr root node.)

newOriginMsg = {
  "@worldmodeltype": "RSGUpdate",
  "operation": "CREATE",
  "node": {
         
    "@graphtype": "Group",
    "id": "853cb0f0-e587-4880-affe-90001da1262d",
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
# denotes the origin node. Though this is a rather arbitrary chioce.
# The root node should be fine as well.
newNodeMsg = {
  "@worldmodeltype": "RSGUpdate",
  "operation": "CREATE",
  "node": {
    "@graphtype": "Node",
    "id": "1c81d51b-e064-463c-8061-5ea822f6f74b", 
    "attributes": [
          {"key": "name", "value": "rescuer"},
    ],
  },
  "parentId": "e379121f-06c6-4e21-ae9d-ae78ec1986a1",
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
      "853cb0f0-e587-4880-affe-90001da1262d",
    ],
    "targetIds": [
      "1c81d51b-e064-463c-8061-5ea822f6f74b",
    ],
    "history" : [
      {
        "stamp": {
          "@stamptype": "TimeStampDate",
          "stamp": "2015-07-01T16:34:26Z",
        },
        "transform": {
          "type": "HomogeneousMatrix44",
            "matrix": [
              [1,0,0,44.153278],
              [0,1,0,12.241426],
              [0,0,1,41.000000],
              [0,0,0,1] 
            ],
            "unit": "latlon"
        }
      }
    ], 	    
  },
  "parentId": "853cb0f0-e587-4880-affe-90001da1262d",
}







# Send message.
time.sleep(1)
print (json.dumps(newOriginMsg))
socket.send_string(json.dumps(newOriginMsg))  
time.sleep(1) 
print (json.dumps(newNodeMsg))
socket.send_string(json.dumps(newNodeMsg))  
time.sleep(1)
print (json.dumps(newTransformMsg))
socket.send_string(json.dumps(newTransformMsg)) 
time.sleep(1) 


