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
getGlobalObservationsNodes = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
      {"key": "name", "value": "global_observations"},
  ]
}
result = json.loads(sendMessageToSWM(json.dumps(getGlobalObservationsNodes)))
ids = result["ids"]
print("ids = %s " % ids)


# Get root node; it used to find that "local" battery; if any
getRootNodeMsg = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_ROOT_NODE"
}
result = json.loads(sendMessageToSWM(json.dumps(getRootNodeMsg)))
rootId = result["rootId"]
print("rootId = %s " % rootId)

getLocalBatteryNodes = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "subgraphId": rootId,
  "attributes": [
      {"key": "sherpa:battery_voltage", "value": "*"},
  ]
}
result = json.loads(sendMessageToSWM(json.dumps(getLocalBatteryNodes)))
batteryIds = result["ids"]
print("batteryIds = %s " % batteryIds)

### Prepare poselist functionblock ###  
blockName = "onattributechange"
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
#result = json.loads(sendMessageToSWM(json.dumps(unloadFunctionBlock))) # optional

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

### Register a monitor listener for ambient_temperature
getPoseListQuery =  {
  "@worldmodeltype": "RSGFunctionBlock",
  "metamodel":       "rsg-functionBlock-schema.json",
  "name":            blockName,
  "operation":       "EXECUTE",
  "input": {
    "metamodel": "fbx-onattributechange-input-schema.json",
	  "monitorOperation": "REGISTER",
    "monitorId": "1ac67940-8639-4b33-b1f5-d9ec3a904da9",
	  "id": ids[0],
    "attributeKey": "ambient_temperature" 
  }
} 
result = json.loads(sendMessageToSWM(json.dumps(getPoseListQuery)))
print(result)  
### Start it
getPoseListQuery =  {
  "@worldmodeltype": "RSGFunctionBlock",
  "metamodel":       "rsg-functionBlock-schema.json",
  "name":            blockName,
  "operation":       "EXECUTE",
  "input": {
    "metamodel": "fbx-onattributechange-input-schema.json",
	  "monitorOperation": "START",
    "monitorId": "1ac67940-8639-4b33-b1f5-d9ec3a904da9"
  }
} 
result = json.loads(sendMessageToSWM(json.dumps(getPoseListQuery)))
print(result)  


### Register a monitor listener for a local battery
getPoseListQuery =  {
  "@worldmodeltype": "RSGFunctionBlock",
  "metamodel":       "rsg-functionBlock-schema.json",
  "name":            blockName,
  "operation":       "EXECUTE",
  "input": {
    "metamodel": "fbx-onattributechange-input-schema.json",
	  "monitorOperation": "REGISTER",
    "monitorId": "ace71788-1b25-48d4-8ccd-17ab820a095e",
	  "id": batteryIds[0],
    "attributeKey": "sherpa:battery_voltage" 
  }
} 
result = json.loads(sendMessageToSWM(json.dumps(getPoseListQuery)))
print(result)  

### Register a monitor listener for a local battery
getPoseListQuery =  {
  "@worldmodeltype": "RSGFunctionBlock",
  "metamodel":       "rsg-functionBlock-schema.json",
  "name":            blockName,
  "operation":       "EXECUTE",
  "input": {
    "metamodel": "fbx-onattributechange-input-schema.json",
	  "monitorOperation": "START",
    "monitorId": "ace71788-1b25-48d4-8ccd-17ab820a095e"
  }
} 
result = json.loads(sendMessageToSWM(json.dumps(getPoseListQuery)))
print(result)  


### Start listening to all incomming monitor messages.
MONITORFUNC = ctypes.CFUNCTYPE(ctypes.c_void_p, ctypes.c_char_p) # Siganture of callback (required for python bindings)

def monitiorCallback(monitorMsg): # The actual custom monitor callback function.
  print("monitorCallback:" + monitorMsg)
  # to be extended here ...

pyMonitiorCallback = MONITORFUNC(monitiorCallback) # make cutome monitor callback adhere the python bindings signatur
swmzyrelib.register_monitor_callback(component, pyMonitiorCallback); # register the callback to the SWM client library. 


### Apply a set of updates to see the monitir in action. 

updateAttributesMsg1 =  {
  "@worldmodeltype": "RSGUpdate",
  "operation": "UPDATE_ATTRIBUTES",
  "updateMode": "UPDATE",
  "node": {
    "@graphtype": "Node",
    "id": "41bba5aa-ebab-45ec-86a9-c37524afd550", 
    "attributes": [
          {"key": "ambient_temperature", "value": "-7C"}
    ]
  }
}

updateAttributesMsg2 =  {
  "@worldmodeltype": "RSGUpdate",
  "operation": "UPDATE_ATTRIBUTES",
  "updateMode": "UPDATE",
  "node": {
    "@graphtype": "Node",
    "id": "41bba5aa-ebab-45ec-86a9-c37524afd550", 
    "attributes": [
          {"key": "ambient_temperature", "value": "-8C"}
    ]
  }
}

sendMessageToSWM(json.dumps(updateAttributesMsg1)) # 1.) -7
sendMessageToSWM(json.dumps(updateAttributesMsg2)) # 2.) -8
sendMessageToSWM(json.dumps(updateAttributesMsg2)) #(-8) this is not a change, so it will not trigger to send a monitor message
sendMessageToSWM(json.dumps(updateAttributesMsg1)) # 3.) -7
sendMessageToSWM(json.dumps(updateAttributesMsg1)) #(-7) this is not a change, so it will not trigger to send a monitor message
sendMessageToSWM(json.dumps(updateAttributesMsg2)) # 4.) -8
sendMessageToSWM(json.dumps(updateAttributesMsg1)) # 5.) -7
sendMessageToSWM(json.dumps(updateAttributesMsg2)) # 6.) -8
sendMessageToSWM(json.dumps(updateAttributesMsg1)) # 7.) -7
sendMessageToSWM(json.dumps(updateAttributesMsg2)) # 8.) -8

#time.sleep(2)

### Clean up
print("Unregister monitor callback")
swmzyrelib.register_monitor_callback(component, 0); # Unregister by passing a 0
swmzyrelib.destroy_component(component);



