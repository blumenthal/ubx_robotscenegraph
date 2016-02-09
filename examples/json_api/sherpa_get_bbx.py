# Example on how to monitor the pose of an object with Attribute (name, <objectValue>)
# In this example the pose is relative to the root node.
# 
# The examples use the JSON API that sends a messages via ZMQ to the SHERPA WM.  

import zmq
import sys
import time
import datetime
import json

### Variables ###

objectAttribute = "osm:way_id"
objectValue = "200" # here we assume that we know the OSM id of the connection that we are intereseted in.

### Communication Setup ###

# Set up the ZMQ REQ-REP communication layer for queries.
context = zmq.Context()
socket = context.socket(zmq.REQ)
swmServer= "tcp://localhost:22422"
socket.connect(swmServer) # Note, currently we have to reconnect for every query, due to an issue in the query server.

### Query to obtain GIS reference origin ####

# Get origin node Id as reference
getOrigin = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
    {"key": "gis:origin", "value": "wgs84" },
  ]
}

print("Sending query for origin node Id: %s " % json.dumps(getOrigin))
socket.send_string(json.dumps(getOrigin))
result = socket.recv()
socket.close()
print("Received reply for origin node Id: %s " % result)

msg = json.loads(result.decode('utf8'))
ids = msg["ids"]
print("ids = %s " % ids)
referenceId = ids[0]
print("referenceId = %s " % referenceId)

### Query to obtain BBX for a Connection with osm:way_id = 200 ####

# Get BBX connection
objectAttribute = "osm:way_ref_id"  
getNodes = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
    {"key": objectAttribute, "value": objectValue },
  ]
}

print("Sending query for object node Id(s): %s " % json.dumps(getNodes))
socket = context.socket(zmq.REQ)
socket.connect(swmServer)
socket.send_string(json.dumps(getNodes))
result = socket.recv()
socket.close()
print("Received reply for object node Id(s): %s " % result)

msg = json.loads(result.decode('utf8'))
ids = msg["ids"]
print("ids = %s " % ids)
bbxConnection = ids[0]
print("bbxConnection = %s " % bbxConnection)


# Get min and max node references.
getBbxNodes = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_CONNECTION_TARGET_IDS",
  "id": bbxConnection
}

print("Sending query for min/may nodes: %s " % json.dumps(getBbxNodes))
socket = context.socket(zmq.REQ)
socket.connect(swmServer)
socket.send_string(json.dumps(getBbxNodes))
result = socket.recv()
print("Received reply for object node Id(s): %s " % result)

msg = json.loads(result.decode('utf8'))
ids = msg["ids"]
print("ids = %s " % ids)
minId = ids[0];
maxId = ids[1];
print("minId = %s " % minId)
print("maxId = %s " % maxId)

# Of course we want to get the _latest_ information.
currentTimeStamp = datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S')

### Get pose for min
getPoseMin = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_TRANSFORM",
  "id": minId,
  "idReferenceNode": referenceId,
  "timeStamp": {
    "@stamptype": "TimeStampDate",
    "stamp": currentTimeStamp,
  } 
}

print("Sending query for min : %s " % json.dumps(getPoseMin))
socket = context.socket(zmq.REQ)
socket.connect(swmServer)
socket.send_string(json.dumps(getPoseMin))
result = socket.recv()
socket.close()
print("Received reply for min: %s " % result)

msg = json.loads(result.decode('utf8'))


### Get pose for max
getPoseMax = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_TRANSFORM",
  "id": maxId,
  "idReferenceNode": referenceId,
  "timeStamp": {
    "@stamptype": "TimeStampDate",
    "stamp": currentTimeStamp,
  } 
}

print("Sending query for max : %s " % json.dumps(getPoseMax))
socket = context.socket(zmq.REQ)
socket.connect(swmServer)
socket.send_string(json.dumps(getPoseMax))
result = socket.recv()
socket.close()
print("Received reply for max: %s " % result)

msg = json.loads(result.decode('utf8'))


