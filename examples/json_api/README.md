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

The examples in this section use either ZMQ REQ-REP (deprecated) as communication layer or Zyre.
The [add_geo_pose.py](add_geo_pose.py) example shows how to seamlessly switch between both choices.
The SHERPA World Model does not depend on this choice. In fact this layer is
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

The examples that use Zyre further require  the [Zyre client communication example](../zyre/README.md) to be installed.  



Usage
-----

### World Model Updates

In order to add data to the world model invoke the following scripts:

```
  python add_node.py 
  python add_pose.py
  python add_geo_pose.py
  ...  
```

Details of the [add_geo_pose.py](add_geo_pose.py) example: 

```
# Example on how to add a new object with an associated pose.
# 
# The examples use the JSON API that sends a messages via
# ZMQ REQ-REP to or Zyre the SHERPA WM.  

import zmq
import random
import sys
import time
import json
import ctypes

###################### Zyre ##################################

### Setup Zyre helper library: swmzyrelib
swmzyrelib = ctypes.CDLL('../zyre/build/libswmzyre.so')
swmzyrelib.wait_for_reply.restype = ctypes.c_char_p
cfg = swmzyrelib.load_config_file('../zyre/swm_zyre_config.json')
component = swmzyrelib.new_component(cfg)

### Define helper method to send a single messagne and receive its reply 
def sendZyreMessageToSWM(message):
  print("Sending message: %s " % (message))
  jsonMsg = swmzyrelib.encode_json_message_from_string(component, message);
  err = swmzyrelib.shout_message(component, jsonMsg);
  result = swmzyrelib.wait_for_reply(component);
  print("Received result: %s " % (result))
  return result

################ ZMQ REQ-REP as alternative ##################

# The port is defined by the system composition (sherpa_world_model.usc) file
# via the ``local_json_in_port`` variable. 
# e.g.
# local local_json_query_port = "22422"  
port = "22422" 

# Set up the ZMQ REQ-REP communication layer.
context = zmq.Context()

def sendZMQMessageToSWM(message):
  socket = context.socket(zmq.REQ)
  socket.connect("tcp://localhost:%s" % port) # Currently he have to reconnect for every message.
  print("Sending update: %s " % (message))
  socket.send_string(message)
  result = socket.recv()
  print("Received result: %s " % (result))
  return result

### Choose the comminucation backend by (un)commentign the prefered method:
def sendMessageToSWM(message):
  #return sendZMQMessageToSWM(message)
  return sendZyreMessageToSWM(message)

##############################################################

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
reply = sendMessageToSWM(json.dumps(newOriginMsg))  
reply = sendMessageToSWM(json.dumps(newNodeMsg))  
reply = sendMessageToSWM(json.dumps(newTransformMsg)) 
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
