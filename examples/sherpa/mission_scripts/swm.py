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
    {"key": "name", "value": "swm"},
  ]
}

objectsQueryMsg = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
    {"key": "name", "value": "objects"},
  ]
}

animalsQueryMsg = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
    {"key": "name", "value": "animals"},
  ]
}

geniusQueryMsg = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
    {"key": "sherpa:agent_name", "value": "genius"},
  ]
}

donkeyQueryMsg = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
    {"key": "sherpa:agent_name", "value": "donkey"},
  ]
}

geniusGeoposeQueryMsg = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
    {"key": "name", "value": "genius geopose"},
  ]
}

donkeyGeoposeQueryMsg = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
    {"key": "name", "value": "donkey geopose"},
  ]
}

def addGroupNode(nodeName, nodeDescription, nodeParentId):
    print "[DCM Interface:] adding the %s node ..." % (nodeName)
#    socket = context.socket(zmq.PUB)
#    pubSocket.bind("tcp://*:%s" % port)
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
    pubSocket.send_string(json.dumps(nodeMsg))  
    print "[DCM Interface:] the %s node was successfully added!" % (nodeName)
    time.sleep(1) 

def addAgentNode(nodeName, nodeDescription, nodeParentId):
    print "[DCM Interface:] adding the %s node ..." % (nodeName)
#    socket = context.socket(zmq.PUB)
#    socket.bind("tcp://*:%s" % port)
    nodeMsg = {
      "@worldmodeltype": "RSGUpdate",
      "operation": "CREATE",
      "node": {
        "@graphtype": "Group",
        "attributes": [
              {"key": "sherpa:agent_name", "value": nodeName},
              {"key": "comment", "value": nodeDescription}
        ],
      },
      "parentId": nodeParentId,
    }
    time.sleep(1)
    pubSocket.send_string(json.dumps(nodeMsg))  
    print "[DCM Interface:] the %s node was successfully added!" % (nodeName)
    time.sleep(1) 

def addGeoposeNode(nodeName, nodeParentId, nodeLat, nodeLon, nodeAlt, nodeQuat0, nodeQuat1, nodeQuat2, nodeQuat3):
    print "[DCM Interface:] adding the geopose node for %s ..." % (nodeName)
#    socket = context.socket(zmq.PUB)
#    socket.bind("tcp://*:%s" % port)
    nodeMsg = {
      "@worldmodeltype": "RSGUpdate",
      "operation": "CREATE",
      "node": {
        "@graphtype": "Node",
        "attributes": [
              {"key": "type", "value": "geopose"},
              {"key": "name", "value": nodeName + " geopose"},
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
    pubSocket.send_string(json.dumps(nodeMsg))  
    print "[DCM Interface:] the geopose node for %s was successfully added!" % (nodeName)
    time.sleep(1) 

def updateGeoposeNode(nodeName, nodeId, nodeLat, nodeLon, nodeAlt, nodeQuat0, nodeQuat1, nodeQuat2, nodeQuat3):
    print "[DCM Interface:] updating the geopose node for %s ..." % (nodeName)
#    socket = context.socket(zmq.PUB)
#    socket.bind("tcp://*:%s" % port)
    nodeMsg = {
      "@worldmodeltype": "RSGUpdate",
      "operation": "UPDATE_ATTRIBUTES",
      "node": {
        "@graphtype": "Node",
        "id": nodeId,
        "attributes": [
              {"key": "type", "value": "geopose"},
              {"key": "name", "value": nodeName + " geopose"},
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
    pubSocket.send_string(json.dumps(nodeMsg))  
    print "[DCM Interface:] the geopose node for %s was successfully updated!" % (nodeName)
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
    addGroupNode("swm", "This is the SHERPA World Model (SWM) of the DCM.", worldModelAgentId)
    swmId = getNodeId(swmQueryMsg)
    addGroupNode("objects", "This is the node of the objects included in the SWM.", swmId)
    objectsId = getNodeId(objectsQueryMsg)
    addGroupNode("animals", "This is the node of the animals, i.e. agents of the SHERPA team.", objectsId)
    addGroupNode("environment", "This is the node of the scenario environment objects.", objectsId)

def addGeniusNode():
    if not getNodeId(swmQueryMsg):
        print "[DCM Interface:] SWM was not created: starting initialisation procedure for SWM ..."
        initialiseSWM()
        print "[DCM Interface:] the initialisation procedure for SWM was successfully completed!"
    animalsId = getNodeId(animalsQueryMsg)
    addAgentNode("genius", "This is the node of the busy genius, i.e. the human rescuer.", animalsId)

def addDonkeyNode():
    if not getNodeId(swmQueryMsg):
        print "[DCM Interface:] SWM was not created: starting initialisation procedure for SWM ..."
        initialiseSWM()
        print "[DCM Interface:] the initialisation procedure for SWM was successfully completed!"
    animalsId = getNodeId(animalsQueryMsg)
    addAgentNode("donkey", "This is the node of the intelligent donkey, i.e. the robotic rover.", animalsId)

def addWaspNode(waspName):
    if not getNodeId(swmQueryMsg):
        print "[DCM Interface:] SWM was not created: starting initialisation procedure for SWM ..."
        initialiseSWM()
        print "[DCM Interface:] the initialisation procedure for SWM was successfully completed!"
    animalsId = getNodeId(animalsQueryMsg)
    addAgentNode("wasp_" + waspName, "This is the node of the wasp %s, i.e. one of the SHERPA quadrotor drone for low-altitude search & rescue." % waspName, animalsId)

def setGeniusGeopose(bgLat, bgLon, bgAlt, bgQuat0, bgQuat1, bgQuat2, bgQuat3):
    bgGeoposeId = getNodeId(geniusGeoposeQueryMsg)
    if not bgGeoposeId:
        if not getNodeId(geniusQueryMsg):
            print "[DCM Interface:] the genius node is not currently available: starting its creation ..."
            addGeniusNode()
        bgId = getNodeId(geniusQueryMsg)
        addGeoposeNode("genius", bgId, bgLat, bgLon, bgAlt, bgQuat0, bgQuat1, bgQuat2, bgQuat3)
    else:
        print "[DCM Interface:] the genius geopose node was already created: updating the geopose attributes ..."
        updateGeoposeNode("genius", bgGeoposeId, bgLat, bgLon, bgAlt, bgQuat0, bgQuat1, bgQuat2, bgQuat3)

def setDonkeyGeopose(donkeyLat, donkeyLon, donkeyAlt, donkeyQuat0, donkeyQuat1, donkeyQuat2, donkeyQuat3):
    donkeyGeoposeId = getNodeId(donkeyGeoposeQueryMsg)
    if not donkeyGeoposeId:
        if not getNodeId(donkeyQueryMsg):
            print "[DCM Interface:] the donkey node is not currently available: starting its creation ..."
            addDonkeyNode()
        donkeyId = getNodeId(donkeyQueryMsg)
        addGeoposeNode("donkey", donkeyId, donkeyLat, donkeyLon, donkeyAlt, donkeyQuat0, donkeyQuat1, donkeyQuat2, donkeyQuat3)
    else:
        print "[DCM Interface:] the donkey geopose node was already created: updating the geopose attributes ..."
        updateGeoposeNode("donkey", donkeyGeoposeId, donkeyLat, donkeyLon, donkeyAlt, donkeyQuat0, donkeyQuat1, donkeyQuat2, donkeyQuat3)

def setWaspGeopose(waspName, waspLat, waspLon, waspAlt, waspQuat0, waspQuat1, waspQuat2, waspQuat3):
    nodeName = "wasp_" + waspName
    waspGeoposeId = getNodeId({
      "@worldmodeltype": "RSGQuery",
      "query": "GET_NODES",
      "attributes": [
        {"key": "name", "value": nodeName + " geopose"},
      ]
    })
    if not waspGeoposeId:    
        waspQueryMsg = {
          "@worldmodeltype": "RSGQuery",
          "query": "GET_NODES",
          "attributes": [
            {"key": "sherpa:agent_name", "value": nodeName},
          ]
        }
        waspId = getNodeId(waspQueryMsg)
        if not waspId:
            print "[DCM Interface:] the %s node is not currently available: starting its creation ..." % (nodeName)
            addWaspNode(waspName)
        waspId = getNodeId(waspQueryMsg)
        addGeoposeNode(nodeName, waspId, waspLat, waspLon, waspAlt, waspQuat0, waspQuat1, waspQuat2, waspQuat3)
    else:
        print "[DCM Interface:] the %s geopose node was already created: updating the geopose attributes ..." %(nodeName)
        updateGeoposeNode(nodeName, waspGeoposeId, waspLat, waspLon, waspAlt, waspQuat0, waspQuat1, waspQuat2, waspQuat3)

def run(inputCommand):
    global port, worldModelAgentId, context, pubSocket
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
    pubSocket = context.socket(zmq.PUB)
    pubSocket.bind("tcp://*:%s" % port)
#    reqSocket = context.socket(zmq.REQ)
#    reqSocket.connect("tcp://localhost:22422")


    if len(sys.argv) > 1:
        if sys.argv[1] == "initialise" or sys.argv[1] == "initialize":
            if getNodeId(swmQueryMsg):
                print "[DCM Interface:] SWM already initialised!"            
            else:
                print "[DCM Interface:] starting the initialisation procedure for SWM ..."
                initialiseSWM()
                print "[DCM Interface:] the initialisation procedure for SWM was successfully completed!"
        elif sys.argv[1] == "add":
            if len(sys.argv) > 2:
                if sys.argv[2] == "genius":
                    if getNodeId(geniusQueryMsg):
                        print "[DCM Interface:] genius node already added!"            
                    else:
                        addGeniusNode()
                elif sys.argv[2] == "donkey":
                    if getNodeId(donkeyQueryMsg):
                        print "[DCM Interface:] donkey node already added!"            
                    else:
                        addDonkeyNode()
                elif sys.argv[2] == "wasp":
                    if len(sys.argv) > 3:
                        addWaspNode(sys.argv[3])
                    else:
                        print "[DCM Interface:] ERROR: please specify a name for the wasp to be added."
                else:
                    print "[DCM Interface:] ERROR: the object you specified isn't included in the SHERPA agents database. Please check it!"
            else:
                print "[DCM Interface:] ERROR: please specify the SHERPA agent to be added."
        elif sys.argv[1] == "set":
            if len(sys.argv) > 3:
                if sys.argv[2] == "genius" and sys.argv[3] == "geopose":
                    if len(sys.argv) > 10:
                        setGeniusGeopose(sys.argv[4], sys.argv[5], sys.argv[6], sys.argv[7], sys.argv[8], sys.argv[9], sys.argv[10])
                    else:
                        print "[DCM Interface:] ERROR: please specify the latitude [DEG], longitude [DEG], altitude [m] and the quaternion elements to be set."
                elif sys.argv[2] == "donkey" and sys.argv[3] == "geopose":
                    if len(sys.argv) > 10:
                        setDonkeyGeopose(sys.argv[4], sys.argv[5], sys.argv[6], sys.argv[7], sys.argv[8], sys.argv[9], sys.argv[10])
                    else:
                        print "[DCM Interface:] ERROR: please specify the latitude [DEG], longitude [DEG], altitude [m] and the quaternion elements to be set."
                elif sys.argv[2] == "wasp":
                    if len(sys.argv) > 11 and sys.argv[4] == "geopose":
                        setWaspGeopose(sys.argv[3], sys.argv[5], sys.argv[6], sys.argv[7], sys.argv[8], sys.argv[9], sys.argv[10], sys.argv[11])
                    else:
                        print "[DCM Interface:] ERROR: please specify the wasp name to be modified followed by the word ''geopose'' and the latitude [DEG], longitude [DEG], altitude [m] and the quaternion elements to be set."
                else:
                    print "[DCM Interface:] ERROR: the object you specified isn't included in the SHERPA agents database. Please check it!"
            else:
                print "[DCM Interface:] ERROR: to set the geopose of a SHERPA agent specify the SHERPA agent geopose to be modified followed by the word ''geopose'' and the latitude [DEG], longitude [DEG], altitude [m] and the quaternion elements to be set."  
        else:
            print "[DCM Interface:] ERROR: the sequence of arguments used is incorrect. Please check it!"  
    else:
        if inputCommand:
            words = inputCommand.split(' ')
            if words[0] == "initialise" or words[0] == "initialize":
                if getNodeId(swmQueryMsg):
                    print "[DCM Interface:] SWM already initialised!"            
                else:
                    print "[DCM Interface:] starting the initialisation procedure for SWM ..."
                    initialiseSWM()
                    print "[DCM Interface:] the initialisation procedure for SWM was successfully completed!"
            elif words[0] == "add":
                if len(words) > 1:
                    if words[1] == "genius":
                        if getNodeId(geniusQueryMsg):
                            print "[DCM Interface:] genius node already added!"            
                        else:
                            addBusyGeniusNode()
                    elif words[1] == "donkey":
                        if getNodeId(donkeyQueryMsg):
                            print "[DCM Interface:] donkey node already added!"            
                        else:
                            addDonkeyNode()
                    elif words[1] == "wasp":
                        if len(words) > 2:
                            addWaspNode(words[2])
                        else:
                            print "[DCM Interface:] ERROR: please specify a name for the wasp to be added."
                    else:
                        print "[DCM Interface:] ERROR: the object you specified isn't included in the SHERPA database. Please check it!"
                else:
                    print "[DCM Interface:] ERROR: please specify the SHERPA agent to be added."
            elif words[0] == "set":
                if len(words) > 2:
                    if words[1] == "genius" and words[2] == "geopose":
                        if len(words) > 9:
                            setGeniusGeopose(words[3], words[4], words[5], words[6], words[7], words[8], words[9])
                        else:
                            print "[DCM Interface:] ERROR: please specify the latitude [DEG], longitude [DEG], altitude [m] and the quaternion elements to be set."
                    elif words[1] == "donkey" and words[2] == "geopose":
                        if len(words) > 9:
                            setDonkeyGeopose(words[3], words[4], words[5], words[6], words[7], words[8], words[9])
                        else:
                            print "[DCM Interface:] ERROR: please specify the latitude [DEG], longitude [DEG], altitude [m] and the quaternion elements to be set."
                    elif words[1] == "wasp":
                        if len(words) > 10 and words[3] == "geopose":
                            setWaspGeopose(words[2], words[4], words[5], words[6], words[7], words[8], words[9], words[10])
                        else:
                            print "[DCM Interface:] ERROR: please specify the wasp name to be modified followed by the word ''geopose'' and the latitude [DEG], longitude [DEG], altitude [m] and the quaternion elements to be set."
                    else:
                        print "[DCM Interface:] ERROR: the object you specified isn't included in the SHERPA agents database. Please check it!"
                else:
                    print "[DCM Interface:] ERROR: to set the geopose of a SHERPA agent specify the SHERPA agent geopose to be modified followed by the word ''geopose'' and the latitude [DEG], longitude [DEG], altitude [m] and the quaternion elements to be set."  
            else:
                print "[DCM Interface:] ERROR: the sequence of arguments used is incorrect. Please check it!"  
        else:
            print "[DCM Interface:] ERROR: the sequence of arguments used is incorrect. Please check it!"  


if __name__ == '__main__':
    if len(sys.argv) > 1:
        run('')
    else:
        print "[DCM Interface:] ERROR: No command given to SWM: please check the available commands on the user guide."
