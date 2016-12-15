# Example on how to get "print" a Robot Scene Graph into a file.
# Assumes scene_setup() has been calles already!
# More details can be found here:
# https://github.com/blumenthal/brics_3d_function_blocks/blob/master/dotdenerator/README.md

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

### Query to get Ids of object and reference frame ####




# In case a specific supgraph should be displayed
getNode = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
      {"key": "name", "value": "swm"},
  ]
}
result = json.loads(sendMessageToSWM(json.dumps(getNode)))
ids = result["ids"]
print("ids = %s " % ids)

if (len(ids) <= 0): 
  exit()

subgraphId = ids[0]

# In case a supgrah of this agent should be displayed
getRootNodeMsg = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_ROOT_NODE"
}
result = json.loads(sendMessageToSWM(json.dumps(getRootNodeMsg)))
rootId = result["rootId"]
print("rootId = %s " % rootId)


### Prepare dotgenerator functionblock ###  
blockName = "dotgenerator"
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
result = json.loads(sendMessageToSWM(json.dumps(loadFunctionBlock))) # has to be loaded at least onece before

### Trigger dotgenerator. subgraphId, path and dotFileName are optional fields.
saveToDoFileCommand =  {
  "@worldmodeltype": "RSGFunctionBlock",
  "metamodel":       "rsg-functionBlock-schema.json",
  "name":            blockName,
  "operation":       "EXECUTE",
  "input": {
    "metamodel": "fbx-subgraph-and-file-schema.json",
    "subgraphId": rootId,
    "path": "/tmp/",
    "dotFileName": "rsg_dump_1"
  }
} 
result = json.loads(sendMessageToSWM(json.dumps(saveToDoFileCommand)))
print(result)  



### Clean up
swmzyrelib.destroy_component(component);




