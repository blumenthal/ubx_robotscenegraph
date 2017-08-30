# Example on how to get a complete trajectory of a SHERPA agent
# assumed scene_setup() has been calles already!

import zmq
import random
import sys
import time
import json
import uuid
import datetime
import ctypes
import os

### Variables ###
fbxPath = os.environ['FBX_MODULES']
fbxPath = fbxPath + "/lib/"

osmFile = "examples/maps/osm/davos.osm"
demFile = "examples/maps/dem/davos.tif"

# Setup config file
configFile='../zyre/swm_zyre_config.json'
if len(sys.argv) > 1:
    configFile =  sys.argv[1]



###################### Zyre ##################################

### Setup Zyre helper library: swmzyrelib
swmzyrelib = ctypes.CDLL('../zyre/build/libswmzyre.so')
swmzyrelib.wait_for_reply.restype = ctypes.c_char_p
cfg = swmzyrelib.load_config_file(configFile)
component = swmzyrelib.new_component(cfg)

### Define helper method to send a single messagne and receive its reply 
def sendZyreMessageToSWM(message):
  print("Sending message: %s " % (message))
  jsonMsg = swmzyrelib.encode_json_message_from_string(component, message);
  err = swmzyrelib.shout_message(component, jsonMsg);
  timeOutInMilliSec = 5000
  result = swmzyrelib.wait_for_reply(component, jsonMsg, timeOutInMilliSec);
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

################     Application      ##################

### Load and configure necessary function blocks via 

## 0.) Optional: clean up existing blocks RSGFunctionBlock queries

unloadFunctionBlock =  {
  "@worldmodeltype": "RSGFunctionBlock",
  "metamodel":       "rsg-functionBlock-schema.json",
  "name":            "osmloader",
  "operation":       "UNLOAD",
  "input": {
    "metamodel": "rsg-functionBlock-path-schema.json",
    "path":     fbxPath,
    "comment":  "path is the same as the FBX_MODULES environment variable appended with a lib/ folder"
  }
}
result = json.loads(sendMessageToSWM(json.dumps(unloadFunctionBlock)))
print("=== Result = " + str(result['operationSuccess']) + " ===")

unloadFunctionBlock =  {
  "@worldmodeltype": "RSGFunctionBlock",
  "metamodel":       "rsg-functionBlock-schema.json",
  "name":            "demloader",
  "operation":       "UNLOAD",
  "input": {
    "metamodel": "rsg-functionBlock-path-schema.json",
    "path":     fbxPath,
    "comment":  "path is the same as the FBX_MODULES environment variable appended with a lib/ folder"
  }
}
result = json.loads(sendMessageToSWM(json.dumps(unloadFunctionBlock)))
print("=== Result = " + str(result['operationSuccess']) + " ===")

## 1.) load OSM (larady done by default)

loadFunctionBlock =  {
  "@worldmodeltype": "RSGFunctionBlock",
  "metamodel":       "rsg-functionBlock-schema.json",
  "name":            "osmloader",
  "operation":       "LOAD",
  "input": {
    "metamodel": "rsg-functionBlock-path-schema.json",
    "path":     fbxPath,
    "comment":  "path is the same as the FBX_MODULES environment variable appended with a lib/ folder"
  }
}
result = json.loads(sendMessageToSWM(json.dumps(loadFunctionBlock)))
print("=== Result = " + str(result['operationSuccess']) + " ===")

## 2.a) load DEM block

loadFunctionBlock =  {
  "@worldmodeltype": "RSGFunctionBlock",
  "metamodel":       "rsg-functionBlock-schema.json",
  "name":            "demloader",
  "operation":       "LOAD",
  "input": {
    "metamodel": "rsg-functionBlock-path-schema.json",
    "path":     fbxPath,
    "comment":  "path is the same as the FBX_MODULES environment variable appended with a lib/ folder"
  }
}
result = json.loads(sendMessageToSWM(json.dumps(loadFunctionBlock)))
print("=== Result = " + str(result['operationSuccess']) + " ===")

## 2.b) load DEM file

loadDEM = {
  "@worldmodeltype": "RSGFunctionBlock",
  "metamodel":       "rsg-functionBlock-schema.json",
  "name":            "demloader",
  "operation":       "EXECUTE",
  "input": {
    "metamodel": "fbx-demloader-input-schema.json",
    "command":  "LOAD_MAP",
    "file":     "examples/maps/dem/davos.tif"
  }
}
result = json.loads(sendMessageToSWM(json.dumps(loadDEM)))
print("=== Result = " + str(result['operationSuccess']) + " ===")


### Get all forests 
getForests = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
      {"key": "osm:landuse", "value": "forest"},
  ]
}
getForests = { # this is an a priori know forest
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
      {"key": "osm:way_id", "value": "329953521"},
  ]
}
result = json.loads(sendMessageToSWM(json.dumps(getForests)))
ids = result["ids"]
print("=== Forest ids = %s " % ids + " ===")

### Get min max elevation for forest

if (len(ids) <= 0): 
  print("No OSM map available. Please call load_map() in SWM")
  exit()
forestId = ids[0]

for id in ids:
  print (id)
  forestId = id

  getForestElevation = {
    "@worldmodeltype": "RSGFunctionBlock",
    "metamodel":       "rsg-functionBlock-schema.json",
    "name":            "demloader",
    "operation":       "EXECUTE",
    "input": {
      "metamodel": "fbx-demloader-input-schema.json",
      "command":   "GET_MIN_MAX_ELEVATION",
      "areaId":    forestId
    }
  }
  result = json.loads(sendMessageToSWM(json.dumps(getForestElevation)))
  print("=== Result = " + str(result['operationSuccess']) + " ===")


getElevation = {
  "@worldmodeltype": "RSGFunctionBlock",
  "metamodel":       "rsg-functionBlock-schema.json",
  "name":            "demloader",
  "operation":       "EXECUTE",
  "input": {
    "metamodel": "fbx-demloader-input-schema.json",
    "command":   "GET_MIN_MAX_ELEVATION"
  }
}
result = json.loads(sendMessageToSWM(json.dumps(getElevation)))
print("=== Result = " + str(result['operationSuccess']) + " ===")


### Optionally get all agents in the forest (invoke sherpa_publish_agent_pose.py to get a correct answer)
getNodesInAreaQuery =  {
  "@worldmodeltype": "RSGFunctionBlock",
  "metamodel":       "rsg-functionBlock-schema.json",
  "name":            "nodesinarea",
  "operation":       "EXECUTE",
  "input": {
    "metamodel": "fbx-nodesinarea-input-schema.json",
	  "areaId": forestId,
    "attributes": [
      {"key": "sherpa:agent_name", "value": "*"},
    ],
  }
} 
result = json.loads(sendMessageToSWM(json.dumps(getNodesInAreaQuery)))
print("=== Result = " + str(result['operationSuccess']) + " ===")


#time.sleep(1)

### Clean up
swmzyrelib.destroy_component(component);
time.sleep(1)


