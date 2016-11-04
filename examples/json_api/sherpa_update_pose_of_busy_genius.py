# Example on how to update the puse of an agent. If it doe not exist,
# it will be greated.
# In fact we do not store the image itself, rahter than the meta data
# to it, in particular the file location. The SHARPA Mediator
# is then used to retrieve the image, if necessary. 
# 
# The examples use the JSON API that sends a messages via
# ZMQ REQ-REP to or Zyre the SHERPA WM.  

import zmq
import random
import sys
import time
import json
import uuid
import datetime
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

# Variables used for this example
x=44.153278
y=12.241426
z=0
agentName="genius"

##############################################################


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
print("origin ids = %s " % ids)

if (len(ids) > 0): # yes, it exists
  originId = ids[0]

else: # no, we have to add one
  print("Adding new origin node.")
  # JSON message to CREATE a an origin Node, if it doe not exist already. (As child to the root node.)
  originId = str(uuid.uuid4())
  newOriginMsg = {
    "@worldmodeltype": "RSGUpdate",
    "operation": "CREATE",
    "node": {
           
      "@graphtype": "Group",
      "id": originId,
      "attributes": [
        {"key": "gis:origin", "value": "wgs84"},
        {"key": "comment", "value": "Reference frame for geo poses. Use this ID for Transform queries."},
      ],       

    },
    "parentId": "e379121f-06c6-4e21-ae9d-ae78ec1986a1",
  }
  reply = sendMessageToSWM(json.dumps(newOriginMsg))

# Check if a group for all SHERPA "animals" exists already. An image shall be added as child of such a Group 
getObservationGroup = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
      {"key": "name", "value": "animals"},
  ]
}
result = json.loads(sendMessageToSWM(json.dumps(getObservationGroup)))
ids = result["ids"]
print("observation ids = %s " % ids)

if (len(ids) > 0): # yes, it exists
  agentsGroupId = ids[0]
else: # no, we use the application wide root node as fall back 
  agentsGroupId = "e379121f-06c6-4e21-ae9d-ae78ec1986a1" 

# Check if the agent with pose exist already, if not we will create it
getAgent = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
    {"key": "sherpa:agent_name", "value": agentName},
  ]
}
result = json.loads(sendMessageToSWM(json.dumps(getAgent)))
ids = result["ids"]
print("agent ids = %s " % ids)

if (len(ids) > 0): # yes, it exists; so we need to now the ID of its pose
  agentId = ids[0]

  getAgentPose = {
    "@worldmodeltype": "RSGQuery",
    "query": "GET_NODES",
    "attributes": [
        {"key": "name", "value": agentName + "_geopose"}, 
    ]
  }
  result = json.loads(sendMessageToSWM(json.dumps(getAgentPose)))
  ids = result["ids"]
  print("tfId = %s " % ids)

  if (len(ids) == 0): # This should not happen. 
    sys.exit("[ERROR] An agent exits but it has no pose!")

  tfId = ids[0]  

else: # no, we have to add one
  print("Adding new agent.")

  # JSON message to CREATE a new SHERPA agent (animal). 
  # Always use the tag "sherpa:agent_name"!
  agentId = str(uuid.uuid4())
  newImageNodeMsg = {
    "@worldmodeltype": "RSGUpdate",
    "operation": "CREATE",
    "node": {
      "@graphtype": "Node",
      "id": agentId, 
      "attributes": [
        {"key": "sherpa:agent_name", "value": agentName},
      ],
    },
    "parentId": agentsGroupId,
  }

  print("Adding new pose for agent.")

  # JSON message to CREATE a new Transform. Note that the parentId must
  # exist beforehands, otherwise this operation does not succeed.
  currentTimeStamp = datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S')
  tfId = str(uuid.uuid4())
  newTransformMsg = {
    "@worldmodeltype": "RSGUpdate",
    "operation": "CREATE",
    "node": {    
      "@graphtype": "Connection",
      "@semanticContext":"Transform",
      "id": tfId,
      "attributes": [
        {"key": "tf:type", "value": "wgs84"},
        {"key": "name", "value": agentName + "_geopose"},    
      ],
      "sourceIds": [
        originId,
      ],
      "targetIds": [
        agentId,
      ],
      "history" : [
        {
          "stamp": {
            "@stamptype": "TimeStampDate",
            "stamp": currentTimeStamp,
          },
          "transform": {
            "type": "HomogeneousMatrix44",
              "matrix": [
                [1,0,0,x],
                [0,1,0,y],
                [0,0,1,z],
                [0,0,0,1] 
              ],
              "unit": "latlon"
          }
        }
      ], 	    
    },
    "parentId": originId,
  }

  # Send messages
  reply = sendMessageToSWM(json.dumps(newImageNodeMsg))  
  reply = sendMessageToSWM(json.dumps(newTransformMsg)) 

# Finally, here comes a pose update
currentTimeStamp = datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S')
print(tfId)
transforUpdateMsg = {
  "@worldmodeltype": "RSGUpdate",
  "operation": "UPDATE_TRANSFORM",
  "node": {    
    "@graphtype": "Connection",
    "@semanticContext":"Transform",
    "id": tfId,
    "history" : [
      {        
        "stamp": {
          "@stamptype": "TimeStampUTCms",
          "stamp": time.time()*1000.0
        },       
        "transform": {
          "type": "HomogeneousMatrix44",
          "matrix": [
            [1,0,0, x],
            [0,1,0, y],
            [0,0,1, z],
            [0,0,0,1] 
          ],
          "unit": "latlon"
        }
      }
    ], 	    
  }
}
reply = sendMessageToSWM(json.dumps(transforUpdateMsg)) 



