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

# The port is defined by the system composition (sherpa_world_model.usc) file
# via the ``local_json_in_port`` variable. 
# e.g.
# local local_json_in_port = "12911"  
port = "12911" 
if len(sys.argv) > 1:
    port =  sys.argv[1]
    int(port)

# Set up the ZMQ PUB-SUB communication layer.
context = zmq.Context()
socket = context.socket(zmq.PUB)
socket.bind("tcp://*:%s" % port)

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
time.sleep(1)
print (json.dumps(newNodeMsg))
socket.send_string(json.dumps(newNodeMsg))  
time.sleep(1) 


