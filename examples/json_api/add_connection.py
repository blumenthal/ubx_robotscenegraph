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
# local local_json_in_port = "12911"  
port = "12911" 
if len(sys.argv) > 1:
    port =  sys.argv[1]
    int(port)

# Set up the ZMQ PUB-SUB communication layer.
context = zmq.Context()
socket = context.socket(zmq.PUB)
socket.bind("tcp://*:%s" % port)

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
time.sleep(1)
print (json.dumps(newConnectionMsg))
socket.send_string(json.dumps(newConnectionMsg))  
time.sleep(1) 


