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
import uuid

### Communication Setup ###

# Set up the ZMQ REQ-REP communication layer for queries.
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
  return result
# JSON message to CREATE a new Root Node.
# 
#id = "aa835642-b70a-448d-a224-0f3f176dec6b"
id = str(uuid.uuid4())
print (id)
newNodeMsg = {
  "@worldmodeltype": "RSGUpdate",
  "operation": "CREATE_REMOTE_ROOT_NODE",
  "node": {
    "@graphtype": "Group",
    "id": id, 
    "attributes": [
          {"key": "name", "value": "rnd_root_node"},
    ],
  }
}


# Send message.
print (json.dumps(newNodeMsg))
result = sendMessageToSWM(json.dumps(newNodeMsg))  



