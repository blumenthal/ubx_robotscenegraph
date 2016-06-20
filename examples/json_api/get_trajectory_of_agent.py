# Example on how to get a complete trajectory of a SHERPA agent
# 

import zmq
import sys
import time
import datetime
import json

### Variables ###
objectAttribute = "sherpa:agent_name"
agent = "genius"
fbxPath = "/opt/src/sandbox/brics_3d_function_blocks/lib/"

if len(sys.argv) > 2:
    objectAttribute = sys.argv[1]
    agent = sys.argv[2]

print("Reqesting pose history for agent : %s " % (agent))

### Communication Setup ###
port = "22422" 
context = zmq.Context()

def querySWM(queryMessage):
    socket = context.socket(zmq.REQ)
    socket.connect("tcp://localhost:%s" % port)
    socket.send_string(json.dumps(queryMessage))
    queryResult = socket.recv_json()
    print ("Query: " + str(queryMessage))
    print ("Query result: " + str(queryResult))
    if queryResult['querySuccess']:
        return queryResult
    else:
        return ""

def updateSWM(updateMessage):
    socket = context.socket(zmq.REQ)
    socket.connect("tcp://localhost:%s" % port)
    socket.send_string(json.dumps(updateMessage))
    updateResult = socket.recv_json()
    print ("Update: " + str(updateMessage))
    print ("Update result: " + str(updateResult))
    if updateResult['updateSuccess']:
        print ("[DCM updateSWM:] updateSuccess = " + str(updateResult['updateSuccess']))
        return updateResult 
    else:
        return ""

def functionBlockOperation(operation):
    socket = context.socket(zmq.REQ)
    socket.connect("tcp://localhost:%s" % port)
    socket.send_string(json.dumps(operation))
    operationResult = socket.recv_json()
    print ("Operation: " + str(operation))
    print ("Operation result: " + str(operationResult))
    if operationResult['operationSuccess']:
        print ("[DCM updateSWM:] operationSuccess = " + str(operationResult['operationSuccess']))
        return operationResult 
    else:
        return ""

### Query to get Ids of object and reference frame ####

# Get root node Id as reference
getRootNodeMsg = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_ROOT_NODE"
}
result = querySWM(getRootNodeMsg)
rootId = result["rootId"]
print("rootId = %s " % rootId)
referenceId = rootId


# Get object Id 
getNodes = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
    {"key": objectAttribute, "value": agent },
  ]
}

result = querySWM(getNodes)
ids = result["ids"]
print("ids = %s " % ids)

if (len(ids) > 0):

  ### Prepare  poisehistory functionblock ###  
  blockName = "posehistory"
  unloadFunctionBlock =  {
    "@worldmodeltype": "RSGFunctionBlock",
    "metamodel":       "rsg-functionBlock-schema.json",
    "name":            blockName,
    "operation":       "UNLOAD",
    "input": {
      "metamodel": "rsg-functionBlock-path-schema.json",
      "path":     fbxPath,
      "comment":  "path is the same as the FBX_MODULES environment variable appended with a lib/ folder"
    }
  } 
  functionBlockOperation(unloadFunctionBlock)

  loadFunctionBlock =  {
    "@worldmodeltype": "RSGFunctionBlock",
    "metamodel":       "rsg-functionBlock-schema.json",
    "name":            blockName,
    "operation":       "LOAD",
    "input": {
      "metamodel": "rsg-functionBlock-path-schema.json",
      "path":     fbxPath,
      "comment":  "path is the same as the FBX_MODULES environment variable appended with a lib/ folder"
    }
  }
  functionBlockOperation(loadFunctionBlock)

  ### Request history ###
  getPoseHistoryQuery =  {
    "@worldmodeltype": "RSGFunctionBlock",
    "metamodel":       "rsg-functionBlock-schema.json",
    "name":            blockName,
    "operation":       "EXECUTE",
    "input": {
      "metamodel": "fbx-posehistory-input-schema.json",
		  "id": ids[0],
		  "idReferenceNode": rootId
    }
  } 
  result = functionBlockOperation(getPoseHistoryQuery)
  print("Recieved pose history:")  
  print(result["output"]["history"])  

  ## For comparison - below is an example how to get a single pose rather than a full history:
  #currentTimeStamp = datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S')
  #print(currentTimeStamp)
  #  
  #getPose = {
  #    "@worldmodeltype": "RSGQuery",
  #    "query": "GET_TRANSFORM",
  #    "id": ids[0],
  #    "idReferenceNode": rootId,
  #    "timeStamp": {
  #      "@stamptype": "TimeStampDate",
  #      "stamp": currentTimeStamp,
  #    } 
  #}
  #result = querySWM(getPose)
  #print("Received reply for object pose: %s " % str(result))



