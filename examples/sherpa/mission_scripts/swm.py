# INTERFACE TO Dynamical Cognitive Map (DCM) Sherpa World Model (SWM)
# TODO: complete the description as in the template below.
# Example on how to add a new and rather generic node into the
# Robot Scene Graph data model that is used within a SHERPA WM.
#
# The examples use the JSON API that sends a single message via
# ZMQ to the SHERPA WM.  

import os.path
import ConfigParser
import zmq
import random
import sys
import time
import json

swmQueryMsg = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
    {"key": "name", "value": "SWM"},
  ]
}

objectsQueryMsg = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
    {"key": "name", "value": "Objects"},
  ]
}

animalsQueryMsg = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
    {"key": "name", "value": "Animals"},
  ]
}

bgQueryMsg = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
    {"key": "name", "value": "Busy Genius"},
  ]
}

donkeyQueryMsg = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
    {"key": "name", "value": "Donkey"},
  ]
}

bgGeoposeQueryMsg = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
    {"key": "name", "value": "Busy Genius Geopose"},
  ]
}

donkeyGeoposeQueryMsg = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
    {"key": "name", "value": "Donkey Geopose"},
  ]
}

def addGroupNode(nodeName, nodeDescription, nodeParentId):
    print "[DCM Interface:] adding the %s node ..." % (nodeName)
    socket = context.socket(zmq.PUB)
    socket.bind("tcp://*:%s" % port)
    nodeMsg = {
      "@worldmodeltype": "RSGUpdate",
      "operation": "CREATE",
      "node": {
        "@graphtype": "Group",
        "attributes": [
              {"key": "name", "value": nodeName},
              {"key": "comment", "value": nodeDescription}
        ],
      },
      "parentId": nodeParentId,
    }
    time.sleep(1)
    socket.send_string(json.dumps(nodeMsg))  
    print "[DCM Interface:] the %s node was successfully added!" % (nodeName)
    time.sleep(1) 

def addGeoposeNode(nodeName, nodeParentId, nodeLat, nodeLon, nodeAlt, nodeQuat0, nodeQuat1, nodeQuat2, nodeQuat3):
    print "[DCM Interface:] adding the Geopose node for %s ..." % (nodeName)
    socket = context.socket(zmq.PUB)
    socket.bind("tcp://*:%s" % port)
    nodeMsg = {
      "@worldmodeltype": "RSGUpdate",
      "operation": "CREATE",
      "node": {
        "@graphtype": "Node",
        "attributes": [
              {"key": "type", "value": "geopose"},
              {"key": "name", "value": nodeName + " Geopose"},
              {"key": "comment", "value": "This is the node defining the geographic position and attitude of %s." % (nodeName)},
              {"key": "latitude", "value": nodeLat}, # WGS84 latitude in degrees (as in float64)
              {"key": "longitude", "value": nodeLon},# WGS84 longitude in degrees (as in float64)
              {"key": "altitude", "value": nodeAlt}, # WGS84 altitude in meters AMSL (as in float64)
              {"key": "q0", "value": nodeQuat0}, # vector element n.1 (as in float64)
              {"key": "q1", "value": nodeQuat1}, # vector element n.2 (as in float64)
              {"key": "q2", "value": nodeQuat2}, # vector element n.3 (as in float64)
              {"key": "q3", "value": nodeQuat3}, # scalar value (as in float64)
              {"key": "timeStamp", "value": time.strftime("%Y-%m-%dT%H:%M:%S%Z")}
        ],
      },
      "parentId": nodeParentId,
    }
    time.sleep(1)
    socket.send_string(json.dumps(nodeMsg))  
    print "[DCM Interface:] the Geopose node for %s was successfully added!" % (nodeName)
    time.sleep(1) 

def updateGeoposeNode(nodeName, nodeId, nodeLat, nodeLon, nodeAlt, nodeQuat0, nodeQuat1, nodeQuat2, nodeQuat3):
    print "[DCM Interface:] updating the Geopose node for %s ..." % (nodeName)
    socket = context.socket(zmq.PUB)
    socket.bind("tcp://*:%s" % port)
    nodeMsg = {
      "@worldmodeltype": "RSGUpdate",
      "operation": "UPDATE_ATTRIBUTES",
      "node": {
        "@graphtype": "Node",
        "id": nodeId,
        "attributes": [
              {"key": "type", "value": "geopose"},
              {"key": "name", "value": nodeName + " Geopose"},
              {"key": "comment", "value": "This is the node defining the geographic position and attitude of %s." % (nodeName)},
              {"key": "latitude", "value": nodeLat}, # WGS84 latitude in degrees (as in float64)
              {"key": "longitude", "value": nodeLon},# WGS84 longitude in degrees (as in float64)
              {"key": "altitude", "value": nodeAlt}, # WGS84 altitude in meters AMSL (as in float64)
              {"key": "q0", "value": nodeQuat0}, # vector element n.1 (as in float64)
              {"key": "q1", "value": nodeQuat1}, # vector element n.2 (as in float64)
              {"key": "q2", "value": nodeQuat2}, # vector element n.3 (as in float64)
              {"key": "q3", "value": nodeQuat3}, # scalar value (as in float64)
              {"key": "timeStamp", "value": time.strftime("%Y-%m-%dT%H:%M:%S%Z")}
        ],
      },
    }
    time.sleep(1)
    socket.send_string(json.dumps(nodeMsg))  
    print "[DCM Interface:] the Geopose node for %s was successfully updated!" % (nodeName)
    time.sleep(1) 

def getNodeId(queryMessage):
    socket = context.socket(zmq.REQ)
    socket.connect("tcp://localhost:22422")
    socket.send_string(json.dumps(queryMessage))
    queryResult = socket.recv_json()
    if queryResult['ids']:
        return queryResult['ids'][0]
    else:
        return ""


def initialiseSWM():
    addGroupNode("SWM", "This is the SHERPA World Model (SWM) of the DCM.", worldModelAgentId)
    swmId = getNodeId(swmQueryMsg)
    addGroupNode("Objects", "This is the node of the objects included in the SWM.", swmId)
    objectsId = getNodeId(objectsQueryMsg)
    addGroupNode("Animals", "This is the node of the animals, i.e. agents of the SHERPA team.", objectsId)
    addGroupNode("Environment", "This is the node of the scenario environment objects.", objectsId)

def addBusyGeniusNode():
    if not getNodeId(swmQueryMsg):
        print "[DCM Interface:] SWM was not created: starting initialisation procedure for SWM ..."
        initialiseSWM()
        print "[DCM Interface:] the initialisation procedure for SWM was successfully completed!"
    animalsId = getNodeId(animalsQueryMsg)
    addGroupNode("Busy Genius", "This is the node of the Busy Genius (BG), i.e. the human rescuer.", animalsId)

def addDonkeyNode():
    if not getNodeId(swmQueryMsg):
        print "[DCM Interface:] SWM was not created: starting initialisation procedure for SWM ..."
        initialiseSWM()
        print "[DCM Interface:] the initialisation procedure for SWM was successfully completed!"
    animalsId = getNodeId(animalsQueryMsg)
    addGroupNode("Donkey", "This is the node of the donkey, i.e. the robotic rover.", animalsId)

def addWaspNode(waspName):
    if not getNodeId(swmQueryMsg):
        print "[DCM Interface:] SWM was not created: starting initialisation procedure for SWM ..."
        initialiseSWM()
        print "[DCM Interface:] the initialisation procedure for SWM was successfully completed!"
    animalsId = getNodeId(animalsQueryMsg)
    addGroupNode("Wasp " + waspName, "This is the node of the wasp %s, i.e. one of the SHERPA quadrotor drone for low-altitude search & rescue." % waspName, animalsId)

def setBusyGeniusGeopose(bgLat, bgLon, bgAlt, bgQuat0, bgQuat1, bgQuat2, bgQuat3):
    bgGeoposeId = getNodeId(bgGeoposeQueryMsg)
    if not bgGeoposeId:
        if not getNodeId(bgQueryMsg):
            print "[DCM Interface:] the Busy Genius node is not currently available: starting its creation ..."
            addBusyGeniusNode()
        bgId = getNodeId(bgQueryMsg)
        addGeoposeNode("Busy Genius", bgId, bgLat, bgLon, bgAlt, bgQuat0, bgQuat1, bgQuat2, bgQuat3)
    else:
        print "[DCM Interface:] the Busy Genius Geopose node was already created: updating the geopose attributes ..."
        updateGeoposeNode("Busy Genius", bgGeoposeId, bgLat, bgLon, bgAlt, bgQuat0, bgQuat1, bgQuat2, bgQuat3)

def setDonkeyGeopose(donkeyLat, donkeyLon, donkeyAlt, donkeyQuat0, donkeyQuat1, donkeyQuat2, donkeyQuat3):
    donkeyGeoposeId = getNodeId(donkeyGeoposeQueryMsg)
    if not donkeyGeoposeId:
        if not getNodeId(donkeyQueryMsg):
            print "[DCM Interface:] the Donkey node is not currently available: starting its creation ..."
            addDonkeyNode()
        donkeyId = getNodeId(donkeyQueryMsg)
        addGeoposeNode("Donkey", donkeyId, donkeyLat, donkeyLon, donkeyAlt, donkeyQuat0, donkeyQuat1, donkeyQuat2, donkeyQuat3)
    else:
        print "[DCM Interface:] the Donkey Geopose node was already created: updating the geopose attributes ..."
        updateGeoposeNode("Donkey", donkeyGeoposeId, donkeyLat, donkeyLon, donkeyAlt, donkeyQuat0, donkeyQuat1, donkeyQuat2, donkeyQuat3)

def setWaspGeopose(waspName, waspLat, waspLon, waspAlt, waspQuat0, waspQuat1, waspQuat2, waspQuat3):
    nodeName = "Wasp " + waspName
    waspGeoposeId = getNodeId({
      "@worldmodeltype": "RSGQuery",
      "query": "GET_NODES",
      "attributes": [
        {"key": "name", "value": nodeName + " Geopose"},
      ]
    })
    if not waspGeoposeId:    
        waspQueryMsg = {
          "@worldmodeltype": "RSGQuery",
          "query": "GET_NODES",
          "attributes": [
            {"key": "name", "value": nodeName},
          ]
        }
        waspId = getNodeId(waspQueryMsg)
        if not waspId:
            print "[DCM Interface:] the %s node is not currently available: starting its creation ..." % (nodeName)
            addWaspNode(waspName)
        waspId = getNodeId(waspQueryMsg)
        addGeoposeNode(nodeName, waspId, waspLat, waspLon, waspAlt, waspQuat0, waspQuat1, waspQuat2, waspQuat3)
    else:
        print "[DCM Interface:] the Wasp Geopose node was already created: updating the geopose attributes ..."
        updateGeoposeNode(nodeName, waspGeoposeId, waspLat, waspLon, waspAlt, waspQuat0, waspQuat1, waspQuat2, waspQuat3)

def run(inputCommand):
    global port, worldModelAgentId, context
#    print "I'm running the run function! :-D"
    # Create the configuration file if it is not available
    configFileName = 'dcm_config.cfg'
    if not os.path.isfile(configFileName):
        print "[DCM Interface Inilialisation:] creating a configuration file ..."
        config = ConfigParser.RawConfigParser()
        config.add_section('Communication')
        config.set('Communication', 'port', '12911')
        config.set('Communication', 'worldModelAgentId', 'e379121f-06c6-4e21-ae9d-ae78ec1986a1')
        with open(configFileName, 'wb') as configFile:
            config.write(configFile)
        print "[DCM Interface Inilialisation:] the configuration file was successfully created!"

    # Read the parameters from the configuration file
    config = ConfigParser.RawConfigParser()
    config.read(configFileName)
    port = config.getint('Communication','port')
    worldModelAgentId = config.get('Communication','worldModelAgentId')
    context = zmq.Context()

    if len(sys.argv) > 1:
        if sys.argv[1] == "initialise" or sys.argv[1] == "initialize":
            if getNodeId(swmQueryMsg):
                print "[DCM Interface:] SWM already initialised!"            
            else:
                print "[DCM Interface:] starting the initialisation procedure for SWM ..."
                initialiseSWM()
                print "[DCM Interface:] the initialisation procedure for SWM was successfully completed!"
        elif sys.argv[1] == "add":
            if sys.argv[2] == "bg":
                if getNodeId(bgQueryMsg):
                    print "[DCM Interface:] Busy Genius node already added!"            
                else:
                    addBusyGeniusNode()
            elif sys.argv[2] == "donkey":
                if getNodeId(donkeyQueryMsg):
                    print "[DCM Interface:] Donkey node already added!"            
                else:
                    addDonkeyNode()
            elif sys.argv[2] == "wasp":
                addWaspNode(sys.argv[3])
            else:
                print "[DCM Interface:] the object you specified isn't included in the SHERPA database. Please check it!"
        elif sys.argv[1] == "set":
            if sys.argv[2] == "bg" and sys.argv[3] == "position":
                setBusyGeniusGeopose(sys.argv[4], sys.argv[5], sys.argv[6], 0, 0, 0, 0)
            elif sys.argv[2] == "donkey":
                setDonkeyGeopose(sys.argv[4], sys.argv[5], sys.argv[6], 0, 0, 0, 0)
            elif sys.argv[2] == "wasp":
                setWaspGeopose(sys.argv[3], sys.argv[5], sys.argv[6], sys.argv[7], 0, 0, 0, 0)
        else:
            print "[DCM Interface:] the sequence of arguments used is incorrect. Please check it!"  
    else:
        words = inputCommand.split(' ')
        if words[0] == "initialise" or words[0] == "initialize":
            if getNodeId(swmQueryMsg):
                print "[DCM Interface:] SWM already initialised!"            
            else:
                print "[DCM Interface:] starting the initialisation procedure for SWM ..."
                initialiseSWM()
                print "[DCM Interface:] the initialisation procedure for SWM was successfully completed!"
        elif words[0] == "add":
            if words[1] == "bg":
                if getNodeId(bgQueryMsg):
                    print "[DCM Interface:] Busy Genius node already added!"            
                else:
                    addBusyGeniusNode()
            elif words[1] == "donkey":
                if getNodeId(donkeyQueryMsg):
                    print "[DCM Interface:] Donkey node already added!"            
                else:
                    addDonkeyNode()
            elif words[1] == "wasp":
                addWaspNode(words[2])
            else:
                print "[DCM Interface:] the object you specified isn't included in the SHERPA database. Please check it!"
        elif words[0] == "set" and words[2] == "geopose":
#            if sys.argv[2] == "bg":
#                setBusyGeniusGeopose(sys.argv[4], sys.argv[5], sys.argv[6])
            if words[1] == "donkey":
                setDonkeyGeopose(words[3], words[4], words[5],words[6], words[7], words[8], words[9])
#            elif sys.argv[2] == "wasp":
#                setWaspGeopose(sys.argv[3], sys.argv[5], sys.argv[6], sys.argv[7])
        else:
            print "[DCM Interface:] the sequence of arguments used is incorrect. Please check it!"  


if __name__ == '__main__':
    run('')
