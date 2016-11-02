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

objectAttribute = "rsg:agent_policy"
objectValue = "send no Atoms from context osm"

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

# 1.) Get root node Id as reference
getRootNodeMsg = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_ROOT_NODE"
}

result = sendMessageToSWM(json.dumps(getRootNodeMsg))
msg = json.loads(result.decode('utf8'))
rootId = msg["rootId"]
print("rootId = %s " % rootId)


# 2.) Get attributes
getAttributesMsg = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODE_ATTRIBUTES",
  "id": rootId
}

result = sendMessageToSWM(json.dumps(getAttributesMsg))
msg = json.loads(result.decode('utf8'))
attributes = msg["attributes"]
print("attributes = %s " % attributes)

# 3.) Append attributes
newAttributes = [
  {"key": objectAttribute, "value": objectValue}
]
#attributes = attributes + newAttributes
attributes.extend(newAttributes)
print("attributes = %s " % attributes)

# 4.) set new attributes
setAttributesMsg = {
  "@worldmodeltype": "RSGUpdate",
  "operation": "UPDATE_ATTRIBUTES",
  "node": {       
    "@graphtype": "Node",
    "id": rootId,
    "attributes": attributes
  }
}
result = sendMessageToSWM(json.dumps(setAttributesMsg))



