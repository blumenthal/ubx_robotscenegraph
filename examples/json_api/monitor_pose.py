# Example on how to monitor the pose of an object with Attribute (name, <objectValue>)
# In this example the pose is relative to the root node.
# 
# The examples use the JSON API that sends a messages via ZMQ to the SHERPA WM.  

import zmq
import sys
import time
import datetime
import json

### Variables ###

objectAttribute = "sherpa:agent_name"
objectValue = "fw0"
server = "tcp://localhost:22422" #local
#server = "tcp://192.168.0.104:22422" #sherpa box
#server = "tcp://127.0.0.1:22422" #local

if len(sys.argv) >= 2:
    objectValue =  sys.argv[1]

if len(sys.argv) >= 3:
    objectValue =  sys.argv[1]
    port = sys.argv[2]
    server = "tcp://localhost:"+port

### Communication Setup ###

# Set up the ZMQ REQ-REP communication layer for queries.
context = zmq.Context()
socket = context.socket(zmq.REQ)
socket.connect(server)

### Query to get Ids of object and reference frame ####

# Get origin 
getOrigin = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
      {"key": "gis:origin", "value": "wgs84"},
  ]
}

print("Sending query for origin node Id: %s " % json.dumps(getOrigin))
socket.send_string(json.dumps(getOrigin))
result = socket.recv()
socket.close()
print("Received reply for origin node Id: %s " % result)
msg = json.loads(result.decode('utf8'))
ids = msg["ids"]

if (len(ids) == 0): 
  print("ERROR. No origin node found. Did you forget to call scene_setup() for the SWM?")
  exit()
else:
  referenceId = ids[0]


# Get object Id 
getNodes = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
    {"key": objectAttribute, "value": objectValue },
  ]
}

print("Sending query for object node Id(s): %s " % json.dumps(getNodes))
socket = context.socket(zmq.REQ)
socket.connect(server)
socket.send_string(json.dumps(getNodes))
result = socket.recv()
socket.close()
print("Received reply for object node Id(s): %s " % result)

msg = json.loads(result.decode('utf8'))
ids = msg["ids"]
print("ids = %s " % ids)

if (len(ids) > 0):
  
  

  ### Poll for updates. ###
  while True: 
    # Of course we want to get the _latest_ information.
    currentTimeStamp = datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S')
    print(currentTimeStamp)
    
    getPose = {
      "@worldmodeltype": "RSGQuery",
      "query": "GET_TRANSFORM",
      "id": ids[0],
      "idReferenceNode": referenceId,
      "timeStamp": {
        "@stamptype": "TimeStampDate",
        "stamp": currentTimeStamp,
      } 
    }

    getPoseTimeStampUTCms = {
      "@worldmodeltype": "RSGQuery",
      "query": "GET_TRANSFORM",
      "id": ids[0],
      "idReferenceNode": referenceId,
      "timeStamp": {
        "@stamptype": "TimeStampUTCms",
        "stamp": time.time()*1000.0,
      },
    }


    socket = context.socket(zmq.REQ)
    socket.connect(server)
    #socket.send_string(json.dumps(getPose))
    socket.send_string(json.dumps(getPoseTimeStampUTCms))
    result = socket.recv()
    socket.close()
    print("Received reply for object pose: %s " % result)
    time.sleep(0.1)


