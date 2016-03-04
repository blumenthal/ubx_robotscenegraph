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
import re

lineNum = 0
#geoPointsNum = 0
#geoPointsLoad = 0
dcmFile = ""

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

environmentQueryMsg = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
    {"key": "name", "value": "environment"},
  ]
}

observationsQueryMsg = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
    {"key": "name", "value": "observations"},
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
    {"key": "name", "value": "genius_geopose"},
  ]
}

donkeyGeoposeQueryMsg = {
  "@worldmodeltype": "RSGQuery",
  "query": "GET_NODES",
  "attributes": [
    {"key": "name", "value": "donkey_geopose"},
  ]
}

def quat2DCM(qx, qy, qz, qs):
    matrix = [[0 for x in range(3)] for x in range(3)]
    matrix[0][0] = 1-2*(qy*qy+qz*qz)
    matrix[0][1] = 2*(qx*qy-qs*qz)
    matrix[0][2] = 2*(qx*qz+qs*qy)
    matrix[1][0] = 2*(qx*qy+qs*qz)
    matrix[1][1] = 1-2*(qx*qx+qz*qz)
    matrix[1][2] = 2*(qy*qz-qs*qx)
    matrix[2][0] = 2*(qx*qz-qs*qy)
    matrix[2][1] = 2*(qy*qz+qs*qx)
    matrix[2][2] = 1-2*(qx*qx+qy*qy)
    return matrix

def addGroupNode(nodeName, nodeDescription, nodeParentId):
    print "[DCM Interface:] adding the %s node ..." % (nodeName)
    time.sleep(1)
    pubSocket.send_string(json.dumps({
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
    }))  
    print "[DCM Interface:] the %s node was successfully added!" % (nodeName)
    time.sleep(1) 

def addAgentNode(nodeName, nodeDescription, nodeParentId):
    print "[DCM Interface:] adding the %s node ..." % (nodeName)
    time.sleep(1)
    pubSocket.send_string(json.dumps({
      "@worldmodeltype": "RSGUpdate",
      "operation": "CREATE",
      "node": {
        "@graphtype": "Node",
        "attributes": [
              {"key": "sherpa:agent_name", "value": nodeName},
              {"key": "comment", "value": nodeDescription}
        ],
      },
      "parentId": nodeParentId,
    }))  
    print "[DCM Interface:] the %s node was successfully added!" % (nodeName)
    time.sleep(1) 

def addGeoPoint(pointParent, pointName, pointParentId, pointLat, pointLon, pointAlt):
    print "[DCM Interface:] adding the point %s of %s ..." % (pointName, pointParent)
    pointFullName = pointParent + "_" + pointName
    time.sleep(1)
    pubSocket.send_string(json.dumps({
      "@worldmodeltype": "RSGUpdate",
      "operation": "CREATE",
      "node": {
        "@graphtype": "Node",
        "attributes": [
          {"key": "name", "value": pointFullName},
          {"key": "osm:node_id", "value": "to be defined"},
        ],
      },
      "parentId": pointParentId,
    }))
    time.sleep(1)
    pointId = getNodeId({
      "@worldmodeltype": "RSGQuery",
      "query": "GET_NODES",
      "attributes": [
          {"key": "name", "value": pointFullName},                      
      ],
      "parentId": pointParentId,
    })
    addGeoposeNode(pointFullName, pointId, pointLat, pointLon, pointAlt, 0, 0, 0, 1)
    print "[DCM Interface:] the point %s of %s was successfully added!" % (pointName, pointParent)

def addGeoposeNode(nodeName, nodeParentId, nodeLat, nodeLon, nodeAlt, nodeQuat0, nodeQuat1, nodeQuat2, nodeQuat3):
    print "[DCM Interface:] adding the geopose node for %s ..." % (nodeName)
    originId = getNodeId({
      "@worldmodeltype": "RSGQuery",
      "query": "GET_NODES",
      "attributes": [
        {"key": "gis:origin", "value": "wgs84"},
      ]
    })
    nodeDcm = quat2DCM(float(nodeQuat0), float(nodeQuat1), float(nodeQuat2), float(nodeQuat3))
    time.sleep(1)
    pubSocket.send_string(json.dumps({
      "@worldmodeltype": "RSGUpdate",
      "operation": "CREATE",
      "node": {
	    "@graphtype": "Connection",
	    "@semanticContext":"Transform",
	    "attributes": [
            {"key": "name", "value": nodeName + "_geopose"},
		    {"key": "tf:type", "value": "wgs84"}
	    ],
	    "sourceIds": [
          originId,
	    ],
	    "targetIds": [
	      nodeParentId
	    ],
	    "history" : [
	      {
	        "stamp": {
	          "@stamptype": "TimeStampDate",
	          "stamp": time.strftime("%Y-%m-%dT%H:%M:%S%Z"),
	        },
	        "transform": {
	          "type": "HomogeneousMatrix44",
	            "matrix": [
                  [nodeDcm[0][0],nodeDcm[0][1],nodeDcm[0][2],nodeLat],
                  [nodeDcm[1][0],nodeDcm[1][1],nodeDcm[1][2],nodeLon],
                  [nodeDcm[2][0],nodeDcm[2][1],nodeDcm[2][2],nodeAlt],
	              [0,0,0,1] 
	            ],
	            "unit": "latlon"
	        }
	      }
	    ], 	    
      },
      "parentId": originId,
    }))  
    print "[DCM Interface:] the geopose node for %s was successfully added!" % (nodeName)
    time.sleep(1) 

def updateGeoposeNode(nodeName, nodeId, nodeLat, nodeLon, nodeAlt, nodeQuat0, nodeQuat1, nodeQuat2, nodeQuat3):
    print "[DCM Interface:] updating the geopose node for %s ..." % (nodeName)
    nodeDcm = quat2DCM(float(nodeQuat0), float(nodeQuat1), float(nodeQuat2), float(nodeQuat3))
    time.sleep(1)
    pubSocket.send_string(json.dumps({
      "@worldmodeltype": "RSGUpdate",
      "operation": "UPDATE_TRANSFORM",
      "node": {
	    "@graphtype": "Connection",
	    "@semanticContext":"Transform",
        "id": nodeId,
	    "history" : [
	      {
	        "stamp": {
	          "@stamptype": "TimeStampDate",
	          "stamp": time.strftime("%Y-%m-%dT%H:%M:%S%Z"),
	        },
	        "transform": {
	          "type": "HomogeneousMatrix44",
	            "matrix": [
                  [nodeDcm[0][0],nodeDcm[0][1],nodeDcm[0][2],nodeLat],
                  [nodeDcm[1][0],nodeDcm[1][1],nodeDcm[1][2],nodeLon],
                  [nodeDcm[2][0],nodeDcm[2][1],nodeDcm[2][2],nodeAlt],
	              [0,0,0,1] 
	            ],
	            "unit": "latlon"
	        }
	      }
	    ], 	    
      }
    }))  
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
    addGroupNode("observations", "This is the node of the the sherpa observations.", objectsId)
    print "[DCM Interface:] adding the geoposes reference frame node ..."
    time.sleep(1)
    pubSocket.send_string(json.dumps({
      "@worldmodeltype": "RSGUpdate",
      "operation": "CREATE",
      "node": {
        "@graphtype": "Group",
        "attributes": [
	      {"key": "gis:origin", "value": "wgs84"},
	      {"key": "comment", "value": "Reference frame for geo poses. Use this ID for Transform queries."},
        ],
      },
      "parentId": swmId,
    }))  
    print "[DCM Interface:] the geoposes reference frame node was successfully added!"
    time.sleep(1) 
    print "[DCM Interface:] the initialisation procedure for SWM was successfully completed!"

def addGeniusNode():
    if not getNodeId(swmQueryMsg):
        print "[DCM Interface:] SWM was not created: starting initialisation procedure for SWM ..."
        initialiseSWM()
    addAgentNode("genius", "This is the node of the busy genius, i.e. the human rescuer.", getNodeId(animalsQueryMsg))

def addDonkeyNode():
    if not getNodeId(swmQueryMsg):
        print "[DCM Interface:] SWM was not created: starting initialisation procedure for SWM ..."
        initialiseSWM()
    addAgentNode("donkey", "This is the node of the intelligent donkey, i.e. the robotic rover.", getNodeId(animalsQueryMsg))

def addWaspNode(waspName):
    waspFullName = "wasp_" + waspName
    if getNodeId({
      "@worldmodeltype": "RSGQuery",
      "query": "GET_NODES",
      "attributes": [
        {"key": "sherpa:agent_name", "value": waspFullName},
      ]
    }):
        print "[DCM Interface:] %s node already added!" % (waspFullName)
    else:           
        if not getNodeId(swmQueryMsg):
            print "[DCM Interface:] SWM was not created: starting initialisation procedure for SWM ..."
            initialiseSWM()
        addAgentNode("wasp_" + waspName, "This is the node of the wasp %s, i.e. one of the SHERPA quadrotor drone for low-altitude search & rescue." % waspName, getNodeId(animalsQueryMsg))

def setGeniusGeopose(bgLat, bgLon, bgAlt, bgQuat0, bgQuat1, bgQuat2, bgQuat3):
    bgGeoposeId = getNodeId(geniusGeoposeQueryMsg)
    if not bgGeoposeId:
        if not getNodeId(geniusQueryMsg):
            print "[DCM Interface:] the genius node is not currently available: starting its creation ..."
            addGeniusNode()
        addGeoposeNode("genius", getNodeId(geniusQueryMsg), bgLat, bgLon, bgAlt, bgQuat0, bgQuat1, bgQuat2, bgQuat3)
    else:
        print "[DCM Interface:] the genius geopose node was already created: updating the geopose attributes ..."
        updateGeoposeNode("genius", bgGeoposeId, bgLat, bgLon, bgAlt, bgQuat0, bgQuat1, bgQuat2, bgQuat3)

def setDonkeyGeopose(donkeyLat, donkeyLon, donkeyAlt, donkeyQuat0, donkeyQuat1, donkeyQuat2, donkeyQuat3):
    donkeyGeoposeId = getNodeId(donkeyGeoposeQueryMsg)
    if not donkeyGeoposeId:
        if not getNodeId(donkeyQueryMsg):
            print "[DCM Interface:] the donkey node is not currently available: starting its creation ..."
            addDonkeyNode()
        addGeoposeNode("donkey", getNodeId(donkeyQueryMsg), donkeyLat, donkeyLon, donkeyAlt, donkeyQuat0, donkeyQuat1, donkeyQuat2, donkeyQuat3)
    else:
        print "[DCM Interface:] the donkey geopose node was already created: updating the geopose attributes ..."
        updateGeoposeNode("donkey", donkeyGeoposeId, donkeyLat, donkeyLon, donkeyAlt, donkeyQuat0, donkeyQuat1, donkeyQuat2, donkeyQuat3)

def setWaspGeopose(waspName, waspLat, waspLon, waspAlt, waspQuat0, waspQuat1, waspQuat2, waspQuat3):
    nodeName = "wasp_" + waspName
    waspGeoposeId = getNodeId({
      "@worldmodeltype": "RSGQuery",
      "query": "GET_NODES",
      "attributes": [
        {"key": "name", "value": nodeName + "_geopose"},
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
        if not getNodeId(waspQueryMsg):
            print "[DCM Interface:] the %s node is not currently available: starting its creation ..." % (nodeName)
            addWaspNode(waspName)
        addGeoposeNode(nodeName, getNodeId(waspQueryMsg), waspLat, waspLon, waspAlt, waspQuat0, waspQuat1, waspQuat2, waspQuat3)
    else:
        print "[DCM Interface:] the %s geopose node was already created: updating the geopose attributes ..." %(nodeName)
        updateGeoposeNode(nodeName, waspGeoposeId, waspLat, waspLon, waspAlt, waspQuat0, waspQuat1, waspQuat2, waspQuat3)

def addPictureNode(pictureAuthor, pictureUrl, pictureLat, pictureLon, pictureAlt, pictureQuat0, pictureQuat1, pictureQuat2, pictureQuat3):
    if not getNodeId(swmQueryMsg):
        print "[DCM Interface:] SWM was not created: starting initialisation procedure for SWM ..."
        initialiseSWM()
        print "[DCM Interface:] the initialisation procedure for SWM was successfully completed!"
    pictureTime = time.strftime("%Y-%m-%dT%H:%M:%S%Z")
    print "[DCM Interface:] adding the picture taken from agent %s at time %s ..." % (pictureAuthor, pictureTime)
    time.sleep(1)
    pubSocket.send_string(json.dumps({
      "@worldmodeltype": "RSGUpdate",
      "operation": "CREATE",
      "node": {
        "@graphtype": "Node",
        "attributes": [
	          {"key": "name", "value": "picture"},
	          {"key": "author", "value": pictureAuthor},
	          {"key": "stamp", "value": pictureTime},
	          {"key": "URL", "value": pictureUrl},
        ],
      },
      "parentId": getNodeId(observationsQueryMsg),
    }))
    time.sleep(1) 
    pictureId = getNodeId({
      "@worldmodeltype": "RSGQuery",
      "query": "GET_NODES",
      "attributes": [
	    {"key": "name", "value": "picture"},
	    {"key": "author", "value": pictureAuthor},
	    {"key": "stamp", "value": pictureTime},
	    {"key": "URL", "value": pictureUrl},
      ]
    })
    addGeoposeNode("picture %s %s" % (pictureAuthor, pictureTime), pictureId, pictureLat, pictureLon, pictureAlt, pictureQuat0, pictureQuat1, pictureQuat2, pictureQuat3)
    print "[DCM Interface:] the picture taken from agent %s at time %s was successfully added!" % (pictureAuthor, pictureTime)
    time.sleep(1) 

def addRiverNode(riverName):
    if getNodeId({
      "@worldmodeltype": "RSGQuery",
      "query": "GET_NODES",
      "attributes": [
          {"key": "type", "value": "river"},
          {"key": "name", "value": riverName},                      
      ]
    }):
        print "[DCM Interface:] %s node already added!" % (riverName)
    else:
        if not getNodeId(swmQueryMsg):
            print "[DCM Interface:] SWM was not created: starting initialisation procedure for SWM ..."
            initialiseSWM()
        print "[DCM Interface:] adding the river %s ..." % (riverName)
        time.sleep(1)
        pubSocket.send_string(json.dumps({
          "@worldmodeltype": "RSGUpdate",
          "operation": "CREATE",
          "node": {
            "@graphtype": "Group",
            "attributes": [
              {"key": "type", "value": "river"},
              {"key": "name", "value": riverName},                      
            ],
          },
          "parentId": getNodeId(environmentQueryMsg),
        }))
        time.sleep(1) 
        print "[DCM Interface:] the river %s was successfully added!" % (riverName)

def addWoodNode(woodName):
    if getNodeId({
      "@worldmodeltype": "RSGQuery",
      "query": "GET_NODES",
      "attributes": [
          {"key": "name", "value": woodName},
          {"key": "osm:natural", "value": "wood"},                      
      ]
    }):
        print "[DCM Interface:] %s node already added!" % (woodName)
    else:
        if not getNodeId(swmQueryMsg):
            print "[DCM Interface:] SWM was not created: starting initialisation procedure for SWM ..."
            initialiseSWM()
        print "[DCM Interface:] adding the wood %s ..." % (woodName)
        time.sleep(1)
        pubSocket.send_string(json.dumps({
          "@worldmodeltype": "RSGUpdate",
          "operation": "CREATE",
          "node": {
            "@graphtype": "Group",
            "attributes": [
              {"key": "name", "value": woodName},
              {"key": "osm:natural", "value": "wood"},                      
            ],
          },
          "parentId": getNodeId(environmentQueryMsg),
        }))
        time.sleep(1) 
        print "[DCM Interface:] the wood %s was successfully added!" % (woodName)

def addHouseNode(houseName):
    if getNodeId({
      "@worldmodeltype": "RSGQuery",
      "query": "GET_NODES",
      "attributes": [
          {"key": "name", "value": houseName},
          {"key": "osm:building", "value": "house"},                      
      ]
    }):
        print "[DCM Interface:] %s node already added!" % (houseName)
    else:
        if not getNodeId(swmQueryMsg):
            print "[DCM Interface:] SWM was not created: starting initialisation procedure for SWM ..."
            initialiseSWM()
        print "[DCM Interface:] adding the house %s ..." % (houseName)
        time.sleep(1)
        pubSocket.send_string(json.dumps({
          "@worldmodeltype": "RSGUpdate",
          "operation": "CREATE",
          "node": {
            "@graphtype": "Group",
            "attributes": [
              {"key": "name", "value": houseName},
              {"key": "osm:building", "value": "house"},                      
            ],
          },
          "parentId": getNodeId(environmentQueryMsg),
        }))
        time.sleep(1) 
        print "[DCM Interface:] the house %s was successfully added!" % (houseName)

def addMountainNode(mountainName):
    if getNodeId({
      "@worldmodeltype": "RSGQuery",
      "query": "GET_NODES",
      "attributes": [
          {"key": "name", "value": mountainName},
          {"key": "osm:natural", "value": "mountain"},                      
      ]
    }):
        print "[DCM Interface:] %s node already added!" % (mountainName)
    else:
        if not getNodeId(swmQueryMsg):
            print "[DCM Interface:] SWM was not created: starting initialisation procedure for SWM ..."
            initialiseSWM()
        print "[DCM Interface:] adding the mountain %s ..." % (mountainName)
        time.sleep(1)
        pubSocket.send_string(json.dumps({
          "@worldmodeltype": "RSGUpdate",
          "operation": "CREATE",
          "node": {
            "@graphtype": "Group",
            "attributes": [
              {"key": "name", "value": mountainName},
              {"key": "osm:natural", "value": "mountain"},                      
            ],
          },
          "parentId": getNodeId(environmentQueryMsg),
        }))
        time.sleep(1) 
        print "[DCM Interface:] the mountain %s was successfully added!" % (mountainName)

def connectRiver(riverName):
    if getNodeId({
      "@worldmodeltype": "RSGQuery",
      "query": "GET_NODES",
      "attributes": [
        {"key": "osm:waterway", "value": "river"},
        {"key": "comment", "value": "This Connection defines all points that belong to %s" % (riverName)},
      ]
    }):
        print "[DCM Interface:] the river %s points were already connected." % (riverName)
    else:
        print "[DCM Interface:] adding connections of river %s points ..." % (riverName)
        socket = context.socket(zmq.REQ)
        socket.connect("tcp://localhost:22422")
        socket.send_string(json.dumps({
          "@worldmodeltype": "RSGQuery",
          "query": "GET_NODES",
          "attributes":[
            {"key": "name", "value": riverName + "_.*"},
            {"key": "osm:node_id", "value": "*"},
	      ]
        }))
        connectionNodes = socket.recv_json()['ids']
        if not connectionNodes:
            exitWithError("no point nodes were found for %s: please add the points of the river boundaries and the compusory bbox1, bbox2 and center points." % (riverName))
            '''
            if lineNum > 0:
                sys.exit("[DCM Interface:] ERROR (%s, line %d): no point nodes were found for %s: please add the points of the river boundaries and the compulsory bbox1, bbox2 and center points." % (dcmFile, lineNum, riverName))  
            else:
                sys.exit("[DCM Interface:] ERROR: no point nodes were found for %s: please add the points of the river boundaries and the compusory bbox1, bbox2 and center points." % (riverName))
            '''
        else:
            parentQueryMsg = {
              "@worldmodeltype": "RSGQuery",
              "query": "GET_NODES",
              "attributes": [
                {"key": "type", "value": "river"},
                {"key": "name", "value": riverName},                      
              ]
            }
            time.sleep(1)
            pubSocket.send_string(json.dumps({
              "@worldmodeltype": "RSGUpdate",
              "operation": "CREATE",
              "node": {     
                "@graphtype": "Connection",
                "attributes": [
                  {"key": "osm:way_id", "value": "to be defined"},
                  {"key": "osm:waterway", "value": "river"},
                  {"key": "comment", "value": "This Connection defines all points that belong to %s" % (riverName)},
                ],
                "sourceIds": [
                ],
                "targetIds": connectionNodes,
                "start": { "@stamptype": "TimeStampUTCms" , "stamp": 0.0 },
                "end": { "@stamptype": "TimeStampDate" , "stamp": "2020-00-00T00:00:00Z" } 
              },
              "parentId": getNodeId(parentQueryMsg)
            }))
            time.sleep(1) 
            print "[DCM Interface:] the river %s connections were successfully added!" % (riverName)
    
def connectWood(woodName):
    if getNodeId({
      "@worldmodeltype": "RSGQuery",
      "query": "GET_NODES",
      "attributes": [
          {"key": "osm:natural", "value": "wood"},
          {"key": "comment", "value": "This Connection defines all points that belong to %s" % (woodName)},
      ]
    }):
        print "[DCM Interface:] the wood %s points were already connected." % (woodName)
    else:
        print "[DCM Interface:] adding connections of wood %s points ..." % (woodName)
        socket = context.socket(zmq.REQ)
        socket.connect("tcp://localhost:22422")
        socket.send_string(json.dumps({
          "@worldmodeltype": "RSGQuery",
          "query": "GET_NODES",
          "attributes":[
       	    {"key": "name", "value": woodName + "_.*"},
            {"key": "osm:node_id", "value": "*"},
    	  ]
        }))
        connectionNodes = socket.recv_json()['ids']
        if not connectionNodes:
            exitWithError("no point nodes were found for %s: please add the points of the wood boundaries and the compulsory bbox1, bbox2 and center points." % (woodName))
        else:
            parentQueryMsg = {
              "@worldmodeltype": "RSGQuery",
              "query": "GET_NODES",
              "attributes": [
                {"key": "name", "value": woodName},
                {"key": "osm:natural", "value": "wood"},                      
              ]
            }
            time.sleep(1)
            pubSocket.send_string(json.dumps({
              "@worldmodeltype": "RSGUpdate",
              "operation": "CREATE",
              "node": {     
                "@graphtype": "Connection",
                "attributes": [
                  {"key": "osm:way_id", "value": "to be defined"},
                  {"key": "osm:natural", "value": "wood"},
                  {"key": "comment", "value": "This Connection defines all points that belong to %s" % (woodName)},
                ],
                "sourceIds": [
                ],
                "targetIds": connectionNodes,
                "start": { "@stamptype": "TimeStampUTCms" , "stamp": 0.0 },
                "end": { "@stamptype": "TimeStampDate" , "stamp": "2020-00-00T00:00:00Z" } 
              },
              "parentId": getNodeId(parentQueryMsg)
            }))
            time.sleep(1) 
            print "[DCM Interface:] the wood %s connections were successfully added!" % (woodName)

def connectHouse(houseName):
    if getNodeId({
      "@worldmodeltype": "RSGQuery",
      "query": "GET_NODES",
      "attributes": [
          {"key": "osm:building", "value": "house"},
          {"key": "comment", "value": "This Connection defines all points that belong to %s" % (houseName)},
      ]
    }):
        print "[DCM Interface:] the house %s points were already connected." % (houseName)
    else:
        print "[DCM Interface:] adding connections of house %s points ..." % (houseName)
        socket = context.socket(zmq.REQ)
        socket.connect("tcp://localhost:22422")
        socket.send_string(json.dumps({
          "@worldmodeltype": "RSGQuery",
          "query": "GET_NODES",
          "attributes":[
            {"key": "name", "value": houseName + "_.*"},
            {"key": "osm:node_id", "value": "*"},
          ]
        }))
        connectionNodes = socket.recv_json()['ids']
        if not connectionNodes:
            exitWithError("no point nodes were found for %s: please add the points of the house boundaries and the compulsory bbox1, bbox2 and center points." % (houseName))
        else:
            parentQueryMsg = {
              "@worldmodeltype": "RSGQuery",
              "query": "GET_NODES",
              "attributes": [
                {"key": "name", "value": houseName},
                {"key": "osm:building", "value": "house"},                      
              ]
            }
            time.sleep(1)
            pubSocket.send_string(json.dumps({
              "@worldmodeltype": "RSGUpdate",
              "operation": "CREATE",
              "node": {     
                "@graphtype": "Connection",
                "attributes": [
                  {"key": "osm:way_id", "value": "to be defined"},
                  {"key": "osm:building", "value": "house"},
                  {"key": "comment", "value": "This Connection defines all points that belong to %s" % (houseName)},
                ],
                "sourceIds": [
                ],
                "targetIds": connectionNodes,
                "start": { "@stamptype": "TimeStampUTCms" , "stamp": 0.0 },
                "end": { "@stamptype": "TimeStampDate" , "stamp": "2020-00-00T00:00:00Z" } 
              },
              "parentId": getNodeId(parentQueryMsg)
            }))
            time.sleep(1) 
            print "[DCM Interface:] the house %s connections were successfully added!" % (houseName)

def connectMountain(mountainName):
    if getNodeId({
      "@worldmodeltype": "RSGQuery",
      "query": "GET_NODES",
      "attributes": [
          {"key": "osm:natural", "value": "mountain"},
          {"key": "comment", "value": "This Connection defines all points that belong to %s" % (mountainName)},
      ]
    }):
        print "[DCM Interface:] the mountain %s points were already connected." % (mountainName)
    else:
        print "[DCM Interface:] adding connections of mountain %s points ..." % (mountainName)
        socket = context.socket(zmq.REQ)
        socket.connect("tcp://localhost:22422")
        socket.send_string(json.dumps({
          "@worldmodeltype": "RSGQuery",
          "query": "GET_NODES",
          "attributes":[
        	{"key": "name", "value": mountainName + "_.*"},
            {"key": "osm:node_id", "value": "*"},
    	  ]
        }))
        connectionNodes = socket.recv_json()['ids']
        if not connectionNodes:
            exitWithError("no point nodes were found for %s: please add the points of the house boundaries and the compulsory bbox1, bbox2 and center points." % (mountainName))
        else:
            parentQueryMsg = {
          "@worldmodeltype": "RSGQuery",
          "query": "GET_NODES",
          "attributes": [
            {"key": "name", "value": mountainName},
            {"key": "osm:natural", "value": "mountain"},                      
          ]
        }
        time.sleep(1)
        pubSocket.send_string(json.dumps({
          "@worldmodeltype": "RSGUpdate",
          "operation": "CREATE",
          "node": {     
            "@graphtype": "Connection",
            "attributes": [
              {"key": "osm:way_id", "value": "to be defined"},
              {"key": "osm:natural", "value": "mountain"},
              {"key": "comment", "value": "This Connection defines all points that belong to %s" % (mountainName)},
            ],
            "sourceIds": [
            ],
            "targetIds": connectionNodes,
            "start": { "@stamptype": "TimeStampUTCms" , "stamp": 0.0 },
            "end": { "@stamptype": "TimeStampDate" , "stamp": "2020-00-00T00:00:00Z" } 
          },
          "parentId": getNodeId(parentQueryMsg)
        }))
        time.sleep(1) 
        print "[DCM Interface:] the mountain %s connections were successfully added!" % (mountainName)

def addRiverPoint(pointParent, pointName, pointLat, pointLon, pointAlt):
    pointFullName = pointParent + "_" + pointName
    if getNodeId({
      "@worldmodeltype": "RSGQuery",
      "query": "GET_NODES",
      "attributes": [
          {"key": "name", "value": pointFullName},
      ]
    }):
        print "[DCM Interface:] the %s node was already created." % (pointFullName)
    else:
        parentQueryMsg = {
          "@worldmodeltype": "RSGQuery",
          "query": "GET_NODES",
          "attributes": [
              {"key": "type", "value": "river"},
              {"key": "name", "value": pointParent},                      
          ]
        }
        if not getNodeId(parentQueryMsg):
            print "[DCM Interface:] the %s node is not currently available: starting its creation ..." % (pointParent)
            addRiverNode(pointParent)
        parentId = getNodeId(parentQueryMsg)
        addGeoPoint(pointParent, pointName, parentId, pointLat, pointLon, pointAlt)

def addWoodPoint(pointParent, pointName, pointLat, pointLon, pointAlt):
    pointFullName = pointParent + "_" + pointName
    if getNodeId({
      "@worldmodeltype": "RSGQuery",
      "query": "GET_NODES",
      "attributes": [
          {"key": "name", "value": pointFullName},
      ]
    }):
        print "[DCM Interface:] the %s node was already created." % (pointFullName)
    else:
        parentQueryMsg = {
          "@worldmodeltype": "RSGQuery",
          "query": "GET_NODES",
          "attributes": [
              {"key": "name", "value": pointParent},
              {"key": "osm:natural", "value": "wood"},                      
          ]
        }
        if not getNodeId(parentQueryMsg):
            print "[DCM Interface:] the %s node is not currently available: starting its creation ..." % (pointParent)
            addWoodNode(pointParent)
        parentId = getNodeId(parentQueryMsg)
        addGeoPoint(pointParent, pointName, parentId, pointLat, pointLon, pointAlt)

def addHousePoint(pointParent, pointName, pointLat, pointLon, pointAlt):
    pointFullName = pointParent + "_" + pointName
    if getNodeId({
      "@worldmodeltype": "RSGQuery",
      "query": "GET_NODES",
      "attributes": [
          {"key": "name", "value": pointFullName},
      ]
    }):
        print "[DCM Interface:] the %s node was already created." % (pointFullName)
    else:
        parentQueryMsg = {
          "@worldmodeltype": "RSGQuery",
          "query": "GET_NODES",
          "attributes": [
            {"key": "name", "value": pointParent},
            {"key": "osm:building", "value": "house"},                      
          ]
        }
        if not getNodeId(parentQueryMsg):
            print "[DCM Interface:] the %s node is not currently available: starting its creation ..." % (pointParent)
            addHouseNode(pointParent)
        parentId = getNodeId(parentQueryMsg)
        addGeoPoint(pointParent, pointName, parentId, pointLat, pointLon, pointAlt)

def addMountainPoint(pointParent, pointName, pointLat, pointLon, pointAlt):
    pointFullName = pointParent + "_" + pointName
    if getNodeId({
      "@worldmodeltype": "RSGQuery",
      "query": "GET_NODES",
      "attributes": [
          {"key": "name", "value": pointFullName},
      ]
    }):
        print "[DCM Interface:] the %s node was already created." % (pointFullName)
    else:
        parentQueryMsg = {
          "@worldmodeltype": "RSGQuery",
          "query": "GET_NODES",
          "attributes": [
            {"key": "name", "value": pointParent},
            {"key": "osm:natural", "value": "mountain"},                      
          ]
        }
        if not getNodeId(parentQueryMsg):
            print "[DCM Interface:] the %s node is not currently available: starting its creation ..." % (pointParent)
            addMountainNode(pointParent)
        parentId = getNodeId(parentQueryMsg)
        addGeoPoint(pointParent, pointName, parentId, pointLat, pointLon, pointAlt)

def stringIsFloat(inputString):
    return inputString.isdigit() or re.match("^\d+?\.\d+?$", inputString) is not None

def stringArrayIsValidGeoPose(inputArray):
    return stringIsFloat(inputArray[0]) and stringIsFloat(inputArray[1]) and stringIsFloat(inputArray[2]) and stringIsFloat(inputArray[3]) and stringIsFloat(inputArray[4]) and stringIsFloat(inputArray[5]) and stringIsFloat(inputArray[6])

def stringArrayIsValidGeoPoint(inputArray):
    return stringIsFloat(inputArray[0]) and stringIsFloat(inputArray[1]) and stringIsFloat(inputArray[2])

def exitWithError(errorMsg):
    if lineNum > 0:
        sys.exit("[DCM Interface:] ERROR (%s, line %d): %s" % (dcmFile, lineNum, errorMsg))  
    else:
        sys.exit("[DCM Interface:] ERROR: %s" % (errorMsg))

def processCommand(words):
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
                    addGeniusNode()
            elif words[1] == "donkey":
                if getNodeId(donkeyQueryMsg):
                    print "[DCM Interface:] donkey node already added!"            
                else:
                    addDonkeyNode()
            elif words[1] == "wasp":
                if len(words) > 2:
                    addWaspNode(words[2])
                else:
                    exitWithError("please specify a name for the wasp to be added!")
                    '''
                    if lineNum > 0:
                        sys.exit("[DCM Interface:] ERROR (%s, line %d): please specify a name for the wasp to be added!" % (dcmFile, lineNum))  
                    else:
                        sys.exit("[DCM Interface:] ERROR: please specify a name for the wasp to be added!")
                    '''
            elif words[1] == "picture":
                if len(words) > 10 and stringArrayIsValidGeoPose(words[4:11]):
                    addPictureNode(words[2], words[3], words[4], words[5], words[6], words[7], words[8], words[9], words[10])
                else:
                    exitWithError("please specify author, URL, latitude [DEG], longitude [DEG], altitude [m] and the quaternion elements of the picture to be added!")
                    '''
                    if lineNum > 0:
                        sys.exit("[DCM Interface:] ERROR (%s, line %d): please specify author, URL, latitude [DEG], longitude [DEG], altitude [m] and the quaternion elements of the picture to be added!" % (dcmFile, lineNum))  
                    else:
                        sys.exit("[DCM Interface:] ERROR: please specify author, URL, latitude [DEG], longitude [DEG], altitude [m] and the quaternion elements of the picture to be added!")
                    '''
            elif words[1] == "river":
                if len(words) > 2:
                    addRiverNode(words[2])
                else:
                    exitWithError("please specify a tag for the river without space characters!")
                    '''
                    if lineNum > 0:
                        sys.exit("[DCM Interface:] ERROR (%s, line %d): please specify a tag for the river without space characters!" % (dcmFile, lineNum))  
                    else:
                        sys.exit("[DCM Interface:] ERROR: please specify a tag for the river without space characters!")
                    '''
            elif words[1] == "wood":
                if len(words) > 2:
                    addWoodNode(words[2])
                else:
                    exitWithError("please specify a tag for the wood without space characters!")
                    '''
                    if lineNum > 0:
                        sys.exit("[DCM Interface:] ERROR (%s, line %d): please specify a tag for the wood without space characters!" % (dcmFile, lineNum))  
                    else:
                        sys.exit("[DCM Interface:] ERROR: please specify a tag for the wood without space characters!")
                    '''
            elif words[1] == "house":
                if len(words) > 2:
                    addHouseNode(words[2])
                else:
                    exitWithError("please specify a tag for the house without space characters!")
                    '''
                    if lineNum > 0:
                        sys.exit("[DCM Interface:] ERROR (%s, line %d): please specify a tag for the house without space characters!" % (dcmFile, lineNum))  
                    else:
                        sys.exit("[DCM Interface:] ERROR: please specify a tag for the house without space characters!")
                    '''
            elif words[1] == "mountain":
                if len(words) > 2:
                    addMountainNode(words[2])
                else:
                    exitWithError("please specify a tag for the mountain without space characters!")
                    '''
                    if lineNum > 0:
                        sys.exit("[DCM Interface:] ERROR (%s, line %d): please specify a tag for the mountain without space characters!" % (dcmFile, lineNum))  
                    else:
                        sys.exit("[DCM Interface:] ERROR: please specify a tag for the mountain without space characters!")
                    '''
            else:
                exitWithError("the object you specified isn't included in the SHERPA database. Please check it!")
                '''
                if lineNum > 0:
                    sys.exit("[DCM Interface:] ERROR (%s, line %d): the object you specified isn't included in the SHERPA database. Please check it!" % (dcmFile, lineNum))  
                else:
                    sys.exit("[DCM Interface:] ERROR: the object you specified isn't included in the SHERPA database. Please check it!")
                '''
        else:
            exitWithError("please specify the SHERPA object to be added!")
            '''
            if lineNum > 0:
                sys.exit("[DCM Interface:] ERROR (%s, line %d): please specify the SHERPA object to be added!" % (dcmFile, lineNum))  
            else:
                sys.exit("[DCM Interface:] ERROR: please specify the SHERPA object to be added!")
            '''
    elif words[0] == "set":
        if len(words) > 2:
            if words[1] == "genius" and words[2] == "geopose":
                if len(words) > 9 and stringArrayIsValidGeoPose(words[3:10]):
                    setGeniusGeopose(words[3], words[4], words[5], words[6], words[7], words[8], words[9])
                else:
                    exitWithError("please specify the latitude [DEG], longitude [DEG], altitude [m] and the quaternion elements to be set.")
                    '''
                    if lineNum > 0:
                        sys.exit("[DCM Interface:] ERROR (%s, line %d): please specify a tag for the mountain without space characters!" % (dcmFile, lineNum))  
                    else:
                        sys.exit("[DCM Interface:] ERROR: please specify the latitude [DEG], longitude [DEG], altitude [m] and the quaternion elements to be set.")
                    '''
            elif words[1] == "donkey" and words[2] == "geopose":
                if len(words) > 9 and stringArrayIsValidGeoPose(words[3:10]):
                    setDonkeyGeopose(words[3], words[4], words[5], words[6], words[7], words[8], words[9])
                else:
                    exitWithError("please specify the latitude [DEG], longitude [DEG], altitude [m] and the quaternion elements to be set!")
                    '''
                    if lineNum > 0:
                        sys.exit("[DCM Interface:] ERROR (%s, line %d): please specify the latitude [DEG], longitude [DEG], altitude [m] and the quaternion elements to be set!" % (dcmFile, lineNum))  
                    else:
                        sys.exit("[DCM Interface:] ERROR: please specify the latitude [DEG], longitude [DEG], altitude [m] and the quaternion elements to be set!")
                    '''
            elif words[1] == "wasp":
                if len(words) > 10 and words[3] == "geopose" and stringArrayIsValidGeoPose(words[4:11]):
                    setWaspGeopose(words[2], words[4], words[5], words[6], words[7], words[8], words[9], words[10])
                else:
                    exitWithError("please specify the wasp name to be modified followed by the word ''geopose'' and the latitude [DEG], longitude [DEG], altitude [m] and the quaternion elements to be set!")
                    '''
                    if lineNum > 0:
                        sys.exit("[DCM Interface:] ERROR (%s, line %d): please specify the wasp name to be modified followed by the word ''geopose'' and the latitude [DEG], longitude [DEG], altitude [m] and the quaternion elements to be set!" % (dcmFile, lineNum))  
                    else:
                        sys.exit("[DCM Interface:] ERROR: please specify the wasp name to be modified followed by the word ''geopose'' and the latitude [DEG], longitude [DEG], altitude [m] and the quaternion elements to be set!")
                    '''
            elif words[1] == "river":
                if len(words) > 3 and words[3] == "connections":
                    connectRiver(words[2])
                elif len(words) > 7 and words[3] == "point" and stringArrayIsValidGeoPoint(words[5:8]):
                    addRiverPoint(words[2],words[4],words[5],words[6],words[7])
                else:
                    exitWithError("the sequence of arguments used is incorrect. Please check it!")
#                    if lineNum > 0:
#                        sys.exit("[DCM Interface:] ERROR (%s, line %d): the sequence of arguments used is incorrect. Please check it!" % (dcmFile, lineNum))  
#                    else:
#                        sys.exit("[DCM Interface:] ERROR: the sequence of arguments used is incorrect. Please check it!")
            elif words[1] == "wood":
                if len(words) > 3 and words[3] == "connections":
                    connectWood(words[2])
                elif len(words) > 7 and words[3] == "point" and stringArrayIsValidGeoPoint(words[5:8]):
                    addWoodPoint(words[2],words[4],words[5],words[6],words[7])
                else:
                    exitWithError("the sequence of arguments used is incorrect. Please check it!")
            elif words[1] == "house":
                if len(words) > 3 and words[3] == "connections":
                    connectHouse(words[2])
                elif len(words) > 7 and words[3] == "point" and stringArrayIsValidGeoPoint(words[5:8]):
                    addHousePoint(words[2],words[4],words[5],words[6],words[7])
                else:
                    exitWithError("the sequence of arguments used is incorrect. Please check it!")
            elif words[1] == "mountain":
                if len(words) > 3 and words[3] == "connections":
                    connectMountain(words[2])
                elif len(words) > 7 and words[3] == "point" and stringArrayIsValidGeoPoint(words[5:8]):
                    addMountainPoint(words[2],words[4],words[5],words[6],words[7])
                else:
                    exitWithError("the sequence of arguments used is incorrect. Please check it!")
            else:
                exitWithError("the object and/or the command you specified isn't included in the SHERPA agents database. Please check it!")
        else:
            exitWithError("the sequence of arguments used is incorrect. Please check it!")
            '''
            if lineNum > 0:
                sys.exit("[DCM Interface:] ERROR (%s, line %d): the sequence of arguments used is incorrect. Please check it!" % (dcmFile, lineNum))  
            else:
                sys.exit("[DCM Interface:] ERROR: the sequence of arguments used is incorrect. Please check it!")  
            '''
    elif words[0] == '':
        exitWithError("please enter the command you want to execute on the DCM!")
#        sys.exit("[DCM Interface:] ERROR: Please enter the command you want to execute on the DCM!")  
    else:
        exitWithError("the sequence of arguments used is incorrect. Please check it!")
        '''
        if lineNum > 0:
            sys.exit("[DCM Interface:] ERROR (%s, line %d): the sequence of arguments used is incorrect. Please check it!" % (dcmFile, lineNum))  
        else:
            sys.exit("[DCM Interface:] ERROR: the sequence of arguments used is incorrect. Please check it!")  
        '''

def run(inputCommand):
            initCom()
            '''
        elif sys.argv[1] == "set":
            if len(sys.argv) > 3:
                elif sys.argv[2] == "river":
                    if sys.argv[4] == "connections":
                        connectRiver(sys.argv[3])
                    elif len(sys.argv) > 8 and sys.argv[4] == "point":
                        addRiverPoint(sys.argv[3],sys.argv[5],sys.argv[6],sys.argv[7],sys.argv[8])
                    else:
                        print "[DCM Interface:] ERROR: the sequence of arguments used is incorrect. Please check it!"
                elif sys.argv[2] == "wood":
                    if sys.argv[4] == "connections":
                        connectWood(sys.argv[3])
                    elif len(sys.argv) > 8 and sys.argv[4] == "point":
                        addWoodPoint(sys.argv[3],sys.argv[5],sys.argv[6],sys.argv[7],sys.argv[8])
                elif sys.argv[2] == "house":
                    if sys.argv[4] == "connections":
                        connectHouse(sys.argv[3])
                    elif len(sys.argv) > 8 and sys.argv[4] == "point":
                        addHousePoint(sys.argv[3],sys.argv[5],sys.argv[6],sys.argv[7],sys.argv[8])
                elif sys.argv[2] == "mountain":
                    if sys.argv[4] == "connections":
                        connectMountain(sys.argv[3])
                    elif len(sys.argv) > 8 and sys.argv[4] == "point":
                        addMountainPoint(sys.argv[3],sys.argv[5],sys.argv[6],sys.argv[7],sys.argv[8])
                else:
                    print "[DCM Interface:] ERROR: the object you specified isn't included in the SHERPA objects database. Please check it!"
            else:
                print "[DCM Interface:] ERROR: to set the geopose of a SHERPA agent specify the SHERPA agent geopose to be modified followed by the word ''geopose'' and the latitude [DEG], longitude [DEG], altitude [m] and the quaternion elements to be set."  
        else:
            sys.exit("[DCM Interface:] ERROR: the sequence of arguments used is incorrect. Please check it!")  
            '''
            processCommand(inputCommand.split(' '))

def initCom():
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


if __name__ == '__main__':
    initCom()
    if len(sys.argv) > 1 and sys.argv[1] == "load":
        if len(sys.argv) > 2:
            fileExt = "dcm"
            dcmFile = sys.argv[2]
            if dcmFile.split(".")[1] == fileExt:
                if os.path.isfile(dcmFile):
                    print "[DCM Interface:] loading file %s on the SWM ..." % (dcmFile)
                    with open(dcmFile) as fp:
                        for line in fp:
                            lineNum = lineNum + 1;
                            line = line.strip(' \n')
                            if line and line[0] != "#":
                                run(line)
                    print "[DCM Interface:] the file %s was successfully loaded on the SWM! ..." % (sys.argv[2])
                else:
                    sys.exit("[DCM Interface:] ERROR: the specified file does not exists!")
            else:
                sys.exit("[DCM Interface:] ERROR: please specify a file with extension .dcm to be loaded!")
        else:
            sys.exit("[DCM Interface:] ERROR: please specify the name of the file with extension .dcm to be loaded!")
    elif len(sys.argv) > 1:
        processCommand(sys.argv[1:])
    else:
        sys.exit("[DCM Interface:] ERROR: No command given to SWM: please check the available commands on the user guide!")
