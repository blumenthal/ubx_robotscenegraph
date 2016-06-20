# Example on how to get a complete trajectory of a SHERPA agent
# 
# to test it call
#  python add_area.py
#  cd ../sherpa/mission_scripts
#  python swm.py add picture genius ok 45 13 2199 0 0 0 1


import zmq
import sys
import time
import datetime
import json
import os

### Variables ###
objectAttribute = "sherpa:agent_name"
agent = "genius"
fbxPath = os.environ['FBX_MODULES']
fbxPath = fbxPath + "/lib/"

if len(sys.argv) > 1:
    agent = sys.argv[1]

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

# Get area object Id 
getAreas = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
      {"key": "geo:area", "value": "polygon"},
  ]
}

result = querySWM(getAreas)
ids = result["ids"]
print("ids = %s " % ids)

if (len(ids) > 0):

  ### Prepare  nodesinarea functionblock ###  
  blockName = "nodesinarea"
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

  ### Request picture nodes witin area ###
  getNodesInAreaQuery =  {
    "@worldmodeltype": "RSGFunctionBlock",
    "metamodel":       "rsg-functionBlock-schema.json",
    "name":            blockName,
    "operation":       "EXECUTE",
    "input": {
      "metamodel": "fbx-nodesinarea-input-schema.json",
		  "areaId": ids[0],
      "attributes": [
        {"key": "name", "value": "picture"},
      ],
    }
  } 
  result = functionBlockOperation(getNodesInAreaQuery)

  pictureIds = result["output"]["ids"]
  if (len(pictureIds) > 0):
    print("Found one or more pictures in the given area.")     
    for i in pictureIds:
     print(i)
     attibutesMsg = {
      "@worldmodeltype": "RSGQuery",
       "query": "GET_NODE_ATTRIBUTES",
        "id": i
     } 
     result = querySWM(attibutesMsg)
     print("attributes = %s " % result["attributes"])       
 


  


