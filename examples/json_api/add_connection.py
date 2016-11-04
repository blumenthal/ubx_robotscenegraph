# Example on how to add a new "Connection" between obejects.
# 
# The examples use the JSON API that sends a messages via
# ZMQ to the SHERPA WM.  

import zmq
import random
import sys
import time
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

## Assumed the cesena_lab has been loaded before.
newConnectionMsg = {
  "@worldmodeltype": "RSGUpdate",
  "operation": "CREATE",
  "node": {    
                                 
    "@graphtype": "Connection",
    "id": "e2b0ac65-0146-4673-9d0a-ff3dd30c4724",
    "attributes": [
      {"key": "osm:way_id", "value": "201"},
      {"key": "osm:highway", "value": "path"},
      {"key": "comment", "value": "This is just an example connection."},
    ],
    "sourceIds": [
    ],
    "targetIds": [
      "62b701e8-1368-458f-9afd-e796f86bfc7a",
      "32b85833-2ff5-49d3-af12-185203b501be",
    ],
    "start": { "@stamptype": "TimeStampUTCms" , "stamp": 0.0 },
    "end": { "@stamptype": "TimeStampDate" , "stamp": "2020-00-00T00:00:00Z" }	    
                              
  },
  "parentId": "e379121f-06c6-4e21-ae9d-ae78ec1986a1",
}

# Send message.
sendMessageToSWM(json.dumps(newConnectionMsg))



