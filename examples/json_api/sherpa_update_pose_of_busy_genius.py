# Example on how to update an existing pose.
# 
# The examples use the JSON API that sends a messages via
# ZMQ to the SHERPA WM.  

import zmq
import random
import sys
import time
import datetime
import json

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

# Of course we want to update a with fresh information generated _now_.
currentTimeStamp = datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S')

# New position to update the pose.
x = random.uniform(44,45)
y = random.uniform(12,13)
z = random.uniform(40,41)

# JSON message to update an existing Transform. Note that the Transform
# (and thus the id)  must
# exist beforehands, otherwise this operation does not succeed.
# in this case the "id": "3304e4a0-44d4-4fc8-8834-b0b03b418d5b"
# denotes the Transform that is cereaten in the add_pose.py example.
transforUpdateMsg = {
  "@worldmodeltype": "RSGUpdate",
  "operation": "UPDATE_TRANSFORM",
  "node": {    
    "@graphtype": "Connection",
    "@semanticContext":"Transform",
    "id": "5786dfa2-6e29-40ae-b90b-c81e8d33aa69",
    "history" : [
      {
        "stamp": {
          "@stamptype": "TimeStampDate",
          "stamp": currentTimeStamp,
        },
        "transform": {
          "type": "HomogeneousMatrix44",
          "matrix": [
            [1,0,0, x],
            [0,1,0, y],
            [0,0,1, z],
            [0,0,0,1] 
          ],
          "unit": "m"
        }
      }
    ], 	    
  }
}

# Send message.
time.sleep(1)
print (json.dumps(transforUpdateMsg))
socket.send_string(json.dumps(transforUpdateMsg))  
time.sleep(1) 


