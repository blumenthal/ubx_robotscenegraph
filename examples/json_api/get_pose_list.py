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
objectAttribute = "sherpa:agent_name"
agent = "genius"
fbxPath = os.environ['FBX_MODULES']
fbxPath = fbxPath + "/lib/"

if len(sys.argv) > 2:
    objectAttribute = sys.argv[1]
    agent = sys.argv[2]

print("Reqesting pose history for agent : %s " % (agent))

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
  return sendZMQMessageToSWM(message)
  #return sendZyreMessageToSWM(message)

### Query to get Ids of object and reference frame ####

# Check if an "origin" node exists already.
getOrigin = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
      {"key": "gis:origin", "value": "wgs84"},
  ]
}
result = json.loads(sendMessageToSWM(json.dumps(getOrigin)))
ids = result["ids"]
print("ids = %s " % ids)

if (len(ids) <= 0): 
  exit()

originId = ids[0]

# Simply query for all existing nodes. In practice this will me a bit more elaborated query...
getAllNodes = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
      {"key": "*", "value": "*"},
  ]
}
result = json.loads(sendMessageToSWM(json.dumps(getAllNodes)))
ids = result["ids"]
print("ids = %s " % ids)

### Prepare poselist functionblock ###  
blockName = "poselist"
unloadFunctionBlock =  {
  "@worldmodeltype": "RSGFunctionBlock",
  "metamodel":       "rsg-functionBlock-schema.json",
  "name":            blockName,
  "operation":       "UNLOAD",
  "input": {
    "metamodel": "rsg-functionBlock-path-schema.json",
    "path":     fbxPath,
    "comment":  "path is the same as the FBX_MODULES environment variable appended with a lib/ folder"
  }
} 
result = json.loads(sendMessageToSWM(json.dumps(unloadFunctionBlock))) # optional

loadFunctionBlock =  {
  "@worldmodeltype": "RSGFunctionBlock",
  "metamodel":       "rsg-functionBlock-schema.json",
  "name":            blockName,
  "operation":       "LOAD",
  "input": {
    "metamodel": "rsg-functionBlock-path-schema.json",
    "path":     fbxPath,
    "comment":  "path is the same as the FBX_MODULES environment variable appended with a lib/ folder"
  }
}
result = json.loads(sendMessageToSWM(json.dumps(loadFunctionBlock))) # has to b eloaded at least onece before

### Request list of poses as defined by "ids" ###
currentTimeStamp = datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S')
getPoseListQuery =  {
  "@worldmodeltype": "RSGFunctionBlock",
  "metamodel":       "rsg-functionBlock-schema.json",
  "name":            blockName,
  "operation":       "EXECUTE",
  "input": {
    "metamodel": "fbx-poselist-input-schema.json",
	  "ids": ids,
	  "idReferenceNode": originId,
    "timeStamp": {
      "@stamptype": "TimeStampDate",
      "stamp": currentTimeStamp,
    } 
  }
} 
result = json.loads(sendMessageToSWM(json.dumps(getPoseListQuery)))
print("Recieved pose history:")  
print(result["output"]["poses"])  

## For comparison - below is an example how to get a single pose rather than a list:
#currentTimeStamp = datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S')
#print(currentTimeStamp)
#  
#getPose = {
#    "@worldmodeltype": "RSGQuery",
#    "query": "GET_TRANSFORM",
#    "id": ids[0],
#    "idReferenceNode": rootId,
#    "timeStamp": {
#      "@stamptype": "TimeStampDate",
#      "stamp": currentTimeStamp,
#    } 
#}
#result = querySWM(getPose)
#print("Received reply for object pose: %s " % str(result))



