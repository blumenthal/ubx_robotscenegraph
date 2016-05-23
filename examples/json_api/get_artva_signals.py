# Example on how to query a existing ARTVA measurements stored in the RSG.
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

def querySWM(query):
  socket = context.socket(zmq.REQ)
  socket.connect("tcp://localhost:%s" % port) # Currently he have to reconnect for every message.
  print("Sending update: %s " % (query))
  socket.send_string(query)
  result = socket.recv()
  print("Received result: %s " % (query))
  return result

### Query to get Ids of object and reference frame ####

# Get root node Id as reference
getRootNodeMsg = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_ROOT_NODE"
}

print("Sending query for root node Id: %s " % json.dumps(getRootNodeMsg))
result = querySWM(json.dumps(getRootNodeMsg));
print("Received reply for root node Id: %s " % result)

msg = json.loads(result.decode('utf8'))
rootId = msg["rootId"]
print("rootId = %s " % rootId)
referenceId = rootId


# Get object Ids of ARTVA measurements 
objectAttribute="sherpa:artva_signal"
objectValue="*"

getNodes = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
    {"key": objectAttribute, "value": objectValue },
  ]
}

print("Sending query for object node Id(s): %s " % json.dumps(getNodes))
result = querySWM(json.dumps(getNodes));
print("Received reply for object node Id(s): %s " % result)

msg = json.loads(result.decode('utf8'))
ids = msg["ids"]
print("ids = %s " % ids)

#if (len(ids) > 0):
for i in ids:  
  
    # Get the actual attributes
    getAttributes = {
      "@worldmodeltype": "RSGQuery",
      "query": "GET_NODE_ATTRIBUTES",
      "id": i 
    }
    result = querySWM(json.dumps(getAttributes))
    print("Received reply for object attributes: %s " % result)

    msg = json.loads(result.decode('utf8'))
    if(msg['attributes']):
      attributes=msg['attributes']
      for i in range(len(attributes)):
       #print i, attributes[i]
       k = attributes[i]['key']
       v = attributes[i]['value'] 
       #print("Key = " + k) 
       if (k==objectAttribute):
        print("*** ARTVA Signal = " + str(v))


    # Of course we want to get the _latest_ information.
    currentTimeStamp = datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S')
    print(currentTimeStamp)
    
    getPose = {
      "@worldmodeltype": "RSGQuery",
      "query": "GET_TRANSFORM",
      "id": i,
      "idReferenceNode": rootId,
      "timeStamp": {
        "@stamptype": "TimeStampDate",
        "stamp": currentTimeStamp,
      } 
    }
    result = querySWM(json.dumps(getPose))
    print("Received reply for object pose: %s " % result)



