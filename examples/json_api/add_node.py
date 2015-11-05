# Example on how to add a new and rather generic node into the
# Robot Scene Graph data model that is used within a SHERPA WM.
#
# The examples use the JSON API that sends a single message via
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

# JSON message to CREATE a new Node. Note that the parentId must
# exist beforehands, otherwise this operation does not succeed.
# in this case the "parentId": "e379121f-06c6-4e21-ae9d-ae78ec1986a1"
# denotes the root id of the graph. It is set in the respective
# system composition (.usc) file and initialized the SHERPA
# world model with exactly that id.
#
# A Node can only be created once. Application of multiple 
# identical CREATE opration will be ignored (and reported as a
# warning)
newNodeMsg = {
  "@worldmodeltype": "RSGUpdate",
  "operation": "CREATE",
  "node": {
    "@graphtype": "Node",
    "id": "92cf7a8d-4529-4abd-b174-5fabbdd3068f", 
    "attributes": [
          {"key": "name", "value": "myNode"},
          {"key": "comment", "value": "Some human readable comment on the node."}
    ],
  },
  "parentId": "e379121f-06c6-4e21-ae9d-ae78ec1986a1",
}
# Anatomy of the message:
#
# The field 
#   "@worldmodeltype": "RSGUpdate",
# is mandatory and denotes the message is an uptate of the
# database for the Robot Scene Graph.
#
#   "operation": "CREATE",
# Defines that this update creates a new node as defined by the
# below node section. Other possibilities are
#
# "CREATE_REMOTE_ROOT_NODE",
# "CREATE_PARENT",
# "UPDATE_ATTRIBUTES",
# "UPDATE_TRANSFORM",
# "UPDATE_START",
# "UPDATE_END",
# "DELETE_NODE",
# "DELETE_PARENT"
#  
# Every CREATE operation needs the definition of a new node:
#  "node": {
#  ...  
#  },
#
# The first element 
#   "@graphtype": "Node", 
# with this definition is the of type of node. Other types can be "Group" or "Connection".
# Every node needs a unique ID, that is specified here as: 
#   "id": "92cf7a8d-4529-4abd-b174-5fabbdd3068f", 
# These IDs are used as primary keys for the Robot Scene Graph.
# If the id field is not set, then a random ID will be assigned automatically.
#
# The attributes field stores a set of attributes via key value pairs. 
# There might be  
#     "attributes": [
#          {"key": "name", "value": "myNode"},
#          {"key": "comment", "value": "Some human readable comment on the node."}
#    ],
#
# The attributes can be used for queries (TODO) to find relevant nodes.
#
# The field 
#
#   "parentId": "e379121f-06c6-4e21-ae9d-ae78ec1986a1",
#
# defined where this node is added in the Robot Scene Graph.
# It can be added to any "Group" node. The root node of
# a World Model Agent is a Group as well. Kie it is done in 
# this example.   

# Send message.
time.sleep(1)
print (json.dumps(newNodeMsg))
socket.send_string(json.dumps(newNodeMsg))  
time.sleep(1) 


