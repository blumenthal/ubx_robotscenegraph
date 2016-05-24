# Example on how to add a command as given by the rescuer.
# One node represents the actual command, while the other one
# it actual interpretation as deduced by the Knowrob reasoning
# engine, leading to an ordered list of actions to be executed by
# a single SHERPA animal(robot). 
# Both nodes are stores as childs of the rescuer 
# (a.k.a "busy genius")
#
# As future extensions the individual actions should be stored as 
# separate Nodes. 
# 
# The examples use the JSON API that sends a messages via
# ZMQ REQ-REP pattern to the SHERPA WM.  

import zmq
import random
import sys
import time
import datetime
import json

# The port is defined by the system composition (sherpa_world_model.usc) file
# via the ``local_json_in_port`` variable. 
# e.g.
# local local_json_query_port = "22422"  
port = "22422" 

# Set up the ZMQ REQ-REP communication layer.
context = zmq.Context()

def sendMessageToSWM(message):
  socket = context.socket(zmq.REQ)
  socket.connect("tcp://localhost:%s" % port) # Currently he have to reconnect for every message.
  print("Sending update: %s " % (message))
  socket.send_string(message)
  result = socket.recv()
  print("Received result: %s " % (result))


# JSON message to CREATE a group for the SHERPA animals i.e. agents of the SHERPA team.
# In this example we will make it a child node of the root node (e379121f-06c6-4e21-ae9d-ae78ec1986a1). 
# Though it might differ for a real application.
newAnimalsGroupMsg = {
  "@worldmodeltype": "RSGUpdate",
  "operation": "CREATE",
  "node": {
        
    "@graphtype": "Group",
    "id": "33ea7c93-2994-4367-8cd0-2e736bba45a5",
    "attributes": [
      {"key": "name", "value": "animals"},
      {"key": "comment", "value": "This is the node of the animals, i.e. agents of the SHERPA team."},
    ],       

  },
  "parentId": "e379121f-06c6-4e21-ae9d-ae78ec1986a1",
}


#ID [33ea7c93-2994-4367-8cd0-2e736bba45a5]
#(name = animals)
#(comment = This is the node of the animals, i.e. agents of the SHERPA team.)

# JSON message to CREATE a group for the SHERPA rescuer 
# In this example we will make it a child node of the above SHERPA animals node.
newGeniusGroupMsg = {
  "@worldmodeltype": "RSGUpdate",
  "operation": "CREATE",
  "node": {        
    "@graphtype": "Group",
    "id": "5def6bbf-4027-40c4-91e5-795088026872",
    "attributes": [
      {"key": "sherpa:agent_name", "value": "genius"},
      {"key": "comment", "value": "This is the node of the busy genius, i.e. the human rescuer."},
    ],       

  },
  "parentId": "33ea7c93-2994-4367-8cd0-2e736bba45a5",
}


# JSON message to CREATE a new command Node as child of the busy genius (cf. above.).
newCommandMsg = {
  "@worldmodeltype": "RSGUpdate",
  "operation": "CREATE",
  "node": {
    "@graphtype": "Node",
    "id": "3e02fe9f-bc0a-4192-86d3-28dfff75f8c9", 
    "attributes": [
          {"key": "sherpa:sherpa:command", "value": "follow me"},
          {"key": "sherpa:target_agent_name", "value": "donkey"},
          {"key": "sherpa:command_pointing_gesture", "value": {"direction":[0.3, 0.7, 0.0]}},
    ],
  },
  "parentId": "5def6bbf-4027-40c4-91e5-795088026872",
}

# JSON message to CREATE a new interpretation/action Node as well as child of the busy genius (cf. above.).
newActionMsg = {
  "@worldmodeltype": "RSGUpdate",
  "operation": "CREATE",
  "node": {
    "@graphtype": "Node",
    "id": "7aa2b497-f605-4bb7-8215-e64e967a0cb3", 
    "attributes": [
          {"key": "sherpa:command_interpetation_success", "value": "true"},
          {"key": "sherpa:target_agent_name", "value": "donkey"},
          {"key": "sherpa:actions", "value": {"actions": [{"action": "move", "geopose": [43.32, 12.3333, 2143]}]} },
    ],
  },
  "parentId": "5def6bbf-4027-40c4-91e5-795088026872",
}


# Send the messages.
sendMessageToSWM(json.dumps(newAnimalsGroupMsg))
sendMessageToSWM(json.dumps(newGeniusGroupMsg))
sendMessageToSWM(json.dumps(newCommandMsg))
sendMessageToSWM(json.dumps(newActionMsg))


