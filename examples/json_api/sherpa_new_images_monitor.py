# Example on how to get set up a monitor that triggers
# whenever a new image (metadata) is inserted.
# In a SHERPA mission a now (geolocated) node is created.
# that hold the relevant metadata, thus we listen for the creation
# of such nodes.

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

### Choose the communication back-end by (un)commenting the preferred method:
def sendMessageToSWM(message):
  #return sendZMQMessageToSWM(message)
  return sendZyreMessageToSWM(message)

### Query to get gis origin as reference frame ####
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

################ Setup monitor ##################

# Work-flow in a nutshell:
# 1.) Load the block, if not done already. It is safe to call it even if it was loaded before. 
#     For testing and debugging purposes it can be useful to always unload first, such that the
#     latest compiled block is used. For an actual rescue mission this can be skipped. 
# 2.) Register the monitor. Here a set of attributes defines which freshly created nodes will 
#     trigger to send a monitor message. A monitorId has to be assigned, that will be used within
#     every message. This allows to filter out monitor messages that do not beling to theis monitor.
#     It is possible to register multiple monitors. E.g on the listens to images and one for victims  
# 3.) START the monitor. A monitor can be also stopped and no further messages will be send unless 
#     it is started again. 
# 4.) Define a custom callback function to react on incoming messages.
# 5.) Use the SWM Zyre Client library to register that callback.  

### 1.) Prepare oncreate monitor function block ### 
blockName = "oncreate"
unloadMonitor =  {
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
result = json.loads(sendMessageToSWM(json.dumps(unloadMonitor))) # optional

loadMonitor =  {
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
result = json.loads(sendMessageToSWM(json.dumps(loadMonitor))) # has to be loaded at least once before

### 2.) Register a monitor listener for new nodes with attribute = (sherpa:observation_type = image)
###     The relevant command is set the filed ``monitorOperation`` to REGISTER,
###     the ``monitorId`` identifies this monitor. It must be a UUID. It has to be the same for all subsequent commands of 
###     REGISTER, UNREGISTER, START and STOP. The monitor messages send from the SWM will have the
###     same id as well. 
getPoseListQuery =  {
  "@worldmodeltype": "RSGFunctionBlock",
  "metamodel":       "rsg-functionBlock-schema.json",
  "name":            blockName,
  "operation":       "EXECUTE",
  "input": {
    "metamodel": "fbx-oncreate-input-schema.json",
	  "monitorOperation": "REGISTER",
    "monitorId": "460b1aa5-78bf-490b-9585-10cf17b6077a",
    "attributes": [
          {"key": "sherpa:observation_type", "value": "image"}
    ] 
  }
} 
result = json.loads(sendMessageToSWM(json.dumps(getPoseListQuery)))
print(result)  

### 3.) Start the monitor. Now ``RSGMonitor`` messages will be send when new image nodes are created. 
###     Note, the ``monitorId`` is the same as uses for the above REGISTER operation. 
getPoseListQuery =  {
  "@worldmodeltype": "RSGFunctionBlock",
  "metamodel":       "rsg-functionBlock-schema.json",
  "name":            blockName,
  "operation":       "EXECUTE",
  "input": {
    "metamodel": "fbx-oncreate-input-schema.json",
	  "monitorOperation": "START",
    "monitorId": "460b1aa5-78bf-490b-9585-10cf17b6077a"
  }
} 
result = json.loads(sendMessageToSWM(json.dumps(getPoseListQuery)))
print(result)  

### 4.) Define a callback whenever a monitor message arrives.
MONITORFUNC = ctypes.CFUNCTYPE(ctypes.c_void_p, ctypes.c_char_p) # Signature of callback (required for python bindings)

def monitiorCallback(monitorMsg): # The actual custom monitor callback function.
  print("monitorCallback:" + monitorMsg)

  # e.g. filter by monitorId 460b1aa5-78bf-490b-9585-10cf17b6077a
  msg = json.loads(monitorMsg)
  monitorId = msg['monitorId']
  print("monitorId = " + monitorId)
  if(monitorId == "460b1aa5-78bf-490b-9585-10cf17b6077a"):
    print("Received a new monitor message for creation of a new image.")
    newNodeid = msg['newNodeid']

    # E.g. get the geo location for that image.
    # WARNING: The geo location can be further updated after creation. 
    # Query the pose again for fresh data, when it is needed. 
    getPose = {
      "@worldmodeltype": "RSGQuery",
      "query": "GET_TRANSFORM",
      "id": newNodeid,
      "idReferenceNode": originId,
      "timeStamp": {
        "@stamptype": "TimeStampUTCms",
        "stamp": time.time()*1000.0,
      },
    }
    result = json.loads(sendMessageToSWM(json.dumps(getPose)))
    print(result)  

    # to be extended here ... 


pyMonitiorCallback = MONITORFUNC(monitiorCallback) # make custom monitor callback adhere the python bindings signature

### 5.) Register the callback
swmzyrelib.register_monitor_callback(component, pyMonitiorCallback); # register the callback to the SWM client library. 



################ Wait for incoming messages ##################

while (True):
  time.sleep(1)

# In order to test the monitor e.g. start the ``sherpa_example`` in the examples/zyre folder 

### Clean up
print("Unregister monitor callback")
swmzyrelib.register_monitor_callback(component, 0); # Unregister by passing a 0
swmzyrelib.destroy_component(component);



