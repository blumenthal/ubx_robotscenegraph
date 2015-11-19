JSON API examples
==================

These examples demonstrate how to store and retrieve data from 
the SHERPA World Model via usage of JSON messages. The JSON messages
allows to perform **C**reate **R**ead **U**pdate and **D**elete *(CRUD)*
 operations on the primitive elements of the Robot Scene Graph. I.e. it is possible to add new
objects to the world model by createing a *Group* or *Node*, adding a 
*Transform* primitive (to represent relative poses between nodes)
and by attaching any additional *Attributes* as needed. Geometric data is 
stored in Geometric Nodes which can be related to any other existing node by
adding a has_geometry relation.   

The examples in this section use ZMQ as communication layer (PUB-SUP for updates and REQ-REP for queries)
though the SHERPA Worde Model does not depend on this choice. In fact this layer is
highly composable due to the use of a [system composition model] (../sherpa/sherpa_world_model.usc). 

Installation 
------------

This example requires the [SHERPA World Model](../../README.md) and Python3 with the ZMQ package to be installed.


### Ubuntu 12.04:
```
	sudo apt-get install python3 python3-setuptools python3-dev
	sudo easy_install3 pip
	pip3 install cffi pyzmq	
```


Usage
-----

### World Model Updates

In order to add data to the world model invoke the following scripts:

```
  python3 add_node.py 
  python3 add_pose.py 
```

Details of the add_node example: 

```
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
# this definition is the of type of node. Other types can be "Group" or "Connection".
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
# The attributes can be used for queries to find relevant nodes.
#
# The field 
#
#   "parentId": "e379121f-06c6-4e21-ae9d-ae78ec1986a1",
#
# defines where this node is added in the Robot Scene Graph.
# It can be added to any "Group" node. The root node of
# a World Model Agent is a Group as well. Kie it is done in 
# this example.   

# Send message.
time.sleep(1)
print (json.dumps(newNodeMsg))
socket.send_string(json.dumps(newNodeMsg))  
time.sleep(1) 
```

Details of the add_pose example: 

```
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

# JSON message to CREATE a new Transform. Note that the parentId must
# exist beforehands, otherwise this operation does not succeed.
# in this case the "parentId": "e379121f-06c6-4e21-ae9d-ae78ec1986a1"
# denotes the root id of the graph. It is set in the respective
# system composition (.usc) file and initialized the SHERPA
newTransformMsg = {
  "@worldmodeltype": "RSGUpdate",
  "operation": "CREATE",
  "node": {    
    "@graphtype": "Connection",
    "@semanticContext":"Transform",
    "id": "3304e4a0-44d4-4fc8-8834-b0b03b418d5b",
    "attributes": [
      {"key": "tf:name", "value": "world_to_rescuer"},
      {"key": "type", "value": "Transform"},
    ],
    "sourceIds": [
      "e379121f-06c6-4e21-ae9d-ae78ec1986a1",
    ],
    "targetIds": [
      "3304e4a0-44d4-4fc8-8834-b0b03b418d5b",
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
            [1,0,0,1.6],
            [0,1,0,1.5],
            [0,0,1,0],
            [0,0,0,1] 
          ],
          "unit": "m"
        }
      }
    ], 	    
  },
  "parentId": "e379121f-06c6-4e21-ae9d-ae78ec1986a1",
}
# Anatomy of the message:
# The field 
#   "@worldmodeltype": "RSGUpdate",
# as seen in the previous example is mandatory and denotes the message is an update of the
# database for the Robot Scene Graph.
#
#   "operation": "CREATE", 
# Defines that this update creates a new graph primitive, while
# the two following lines specify to create an actual Transform (= pose)
#    "@graphtype": "Connection",
#    "@semanticContext":"Transform",
#
# We exlicitly specify a unique ID, that is specified here as: 
#   "id": "3304e4a0-44d4-4fc8-8834-b0b03b418d5b", 
# such that we directly know how to adreess the Transform in 
# order to perform successive updates.
#
# The explary attributes are:
# There might be  
#     "attributes": [
#        {"key": "tf:name", "value": "world_to_rescuer"},
#        {"key": "type", "value": "Transform"},
#    ],
#
# The attributes can be used for queries to find relevant nodes or
# for debugging purposes.
#
# For definition of the the meaning, a single sourceId describes the 
# frecerence frame as the origin of this node. The second node 
# denotes where the Trasform "points" to.
#
# "sourceIds": [
#      "e379121f-06c6-4e21-ae9d-ae78ec1986a1",
#    ],
#    "targetIds": [
#      "3304e4a0-44d4-4fc8-8834-b0b03b418d5b",
#    ],
# 
# The actual data is stored in a temporal cache labelled as
# "history". Is is essantially a list of tuples of time stamps and 
# data for a rigid transformation as represented by a homogeneous
# transformation matrix:
#
#
#   "history" : [
#      {
#        "stamp": {
#          "@stamptype": "TimeStampDate",
#          "stamp": "2015-07-01T16:34:26Z",
#        },
#        "transform": {
#          "type": "HomogeneousMatrix44",
#          "matrix": [
#            [1,0,0,1.6],
#            [0,1,0,1.5],
#            [0,0,1,0],
#            [0,0,0,1] 
#          ],
#          "unit": "m"
#        }
#      }
#    ], 	    
#
# On creation of the Transform it is enough to specify one
# entry. Though a longer list could be provided. The units
# of mesurements indiacation "unit" is optional but highly 
# recommended. Default is [m].
#
#
# The field 
#
#   "parentId": "e379121f-06c6-4e21-ae9d-ae78ec1986a1",
#
# defined where this node is added in the Robot Scene Graph.
# It can be added to any "Group" node. The root node of
# a World Model Agent is a Group as well. Like it is done in 
# this example.   



# JSON message to CREATE a new Node. Note, that the parentId must
# exist beforehands, otherwise this operation does not succeed.
# in this case the "parentId": "3304e4a0-44d4-4fc8-8834-b0b03b418d5b"
# denotes the Trasform that has hust been created.
#
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
  "parentId": "3304e4a0-44d4-4fc8-8834-b0b03b418d5b",
}
# Anatomy of the message:
#
# Please refere to the add_node.py example. The difference is that
# another "id" and another set of attributes is used.
# The transform node is used as parent Id.


# Send message.
time.sleep(1)
print (json.dumps(newTransformMsg))
socket.send_string(json.dumps(newTransformMsg))  
time.sleep(1) 
print (json.dumps(newNodeMsg))
socket.send_string(json.dumps(newNodeMsg))  
time.sleep(1) 
```

### World Model Queries 

Retreive data from the world wodel by invokation of the following commands:

```
  python3 get.py node_attributes_query.json  
  python3 get.py nodes_query.json 
  python3 get.py root_node_query.json 
  python3 get.py transform_query.json  
  python3 get.py geometry_query.json 
```

The get.py example send a JSON message as specified by the command line 
argument. It sends it to the to a ZMQ server that decodes and perfoms the
query and sends back the result in a single JSON message:

```
#   Request-reply client in Python
#   Connects REQ socket to tcp://localhost:22422
#   Sends JSON Request as specifies by a file and
#   print the reply
#
import zmq
import sys

# Setup query
if len(sys.argv) > 1:
    fileName =  sys.argv[1]
    with open (fileName, "r") as messagefile:
      message=messagefile.read()
else:
    message="{}"

#  Prepare our context and sockets
context = zmq.Context()
socket = context.socket(zmq.REQ)
socket.connect("tcp://localhost:22422")
   
print("Sending query: %s " % (message))
socket.send_string(message)
result = socket.recv()
print("Received reult: %s " % (result))
``` 

Anatomy of a query message:

```
{
	"@worldmodeltype": "RSGQuery",
```
Every query must have set ``"@worldmodeltype":`` to ``"RSGQuery"``. While 
the ``query`` field defines the actual type of query e.g.:
```	
	"query": "GET_TRANSFORM",
```
Possibilities are ``GET_NODES, GET_NODE_ATTRIBUTES, GET_NODE_PARENTS, GET_GROUP_CHILDREN,
GET_ROOT_NODE, GET_REMOTE_ROOT_NODES, GET_TRANSFORM`` and ``GET_GEOMETRY``. 
Depending on this type further fields have to be set. The ``GET_TRANSFORM`` query requires
an ``id`` and ``idReferenceNode`` to be set. This defines a *Transform* to be calculated between both
nodes. ``idReferenceNode`` denotes the reference frame. The ``timeStamp`` sets the point 
in time for this query.
```	
	"id": "3304e4a0-44d4-4fc8-8834-b0b03b418d5b",
	"idReferenceNode": "e379121f-06c6-4e21-ae9d-ae78ec1986a1",
	"timeStamp": {
	    "@stamptype": "TimeStampDate",
		"stamp": "2015-11-12T16:16:44Z",
	} 
}
``` 
The following fields are used for the different queries:

| Fields | Description |
|-------|-------------|
| ``id`` | Mandatrory except for GET_NODES query. | 
| ``idReferenceNode`` | Mandatory only for GET_TRANSFORM query. | 
| ``timeStamp`` | Mandatory only for GET_TRANSFORM query. | 
| ``attributes`` | Mandatrory for GET_NODES query. | 


Further examples for queries can in this folder. 


Complete example output from a query:

```
python3 get.py transform_query.json  

Sending query: {
"@worldmodeltype": "RSGQuery",
  "query": "GET_TRANSFORM",
  "id": "3304e4a0-44d4-4fc8-8834-b0b03b418d5b",
  "idReferenceNode": "e379121f-06c6-4e21-ae9d-ae78ec1986a1",
  "timeStamp": {
    "@stamptype": "TimeStampDate",
    "stamp": "2015-11-12T16:16:44Z",
  } 
}
 
Received result: b'{"@worldmodeltype": "RSGQueryResult","query": "GET_TRANSFORM","querySuccess": true,"transform": {"matrix": [[1.0000000000000000,0.0000000000000000,0.0000000000000000,1.6000000000000001],[0.0000000000000000,1.0000000000000000,0.0000000000000000,1.5000000000000000],[0.0000000000000000,0.0000000000000000,1.0000000000000000,0.0000000000000000],[0.0000000000000000,0.0000000000000000,0.0000000000000000,1.0000000000000000]],"type": "HomogeneousMatrix44","unit": "m"}}' 
```  
