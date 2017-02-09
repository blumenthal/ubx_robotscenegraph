# Example on how to add a new area to the RSG.
# An area is represented as a polygon of geo-located Nodes.
# This polygon is stared via a Connection that has at least 
# the following Attribute: (geo:area, polygon). 
# The set of Nodes must be indicated as targetIds. 
# The first and the last Node ID must be the same.
# 
#
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

def sendMessageToSWM(message):
  socket = context.socket(zmq.REQ)
  socket.connect("tcp://localhost:%s" % port) # Currently he have to reconnect for every message.
  print("Sending update: %s " % (message))
  socket.send_string(message)
  result = socket.recv()
  print("Received result: %s " % (result))


coodinates = [ 
  (9.848141855014655,46.815054428015216),
  (9.849936915109629,46.815034548584826),
  (9.849909338796687,46.81412616861588), 
  (9.848140502048613,46.81414549510094), 
  (9.848141855014655,46.815054428015216)
]

# JSON message to CREATE an origin Node. (As child to the root node.)
newOriginMsg = {
  "@worldmodeltype": "RSGUpdate",
  "operation": "CREATE",
  "node": {
         
    "@graphtype": "Group",
    "id": "953cb0f0-e587-4880-affe-90001da1262d",
    "attributes": [
      {"key": "gis:origin", "value": "wgs84"},
      {"key": "comment", "value": "Reference frame for geo poses. Use this ID for Transform queries."},
    ],       

  },
  "parentId": "e379121f-06c6-4e21-ae9d-ae78ec1986a1",
}


# JSON message to CREATE a new Node. Note, that the parentId must
# exist beforehands, otherwise this operation does not succeed.
# in this case the "parentId": "953cb0f0-e587-4880-affe-90001da1262d"
# denotes the origin node. Though this is a rather arbitrary choice.
# The root node should be fine as well. As best practice a common "environment" 
# Group is recommend.
newAreaPolygonPouint1Msg = {
  "@worldmodeltype": "RSGUpdate",
  "operation": "CREATE",
  "node": {
    "@graphtype": "Node",
    "id": "7a47e674-f3c3-47d9-aae3-fd558603076b", 
    "attributes": [
          {"key": "comment", "value": "waypoint 1"},
    ],
  },
  "parentId": "953cb0f0-e587-4880-affe-90001da1262d",
}

newAreaPolygonPouint2Msg = {
  "@worldmodeltype": "RSGUpdate",
  "operation": "CREATE",
  "node": {
    "@graphtype": "Node",
    "id": "2743bbaa-a590-42b7-aa38-d573c18fe6f6", 
    "attributes": [
          {"key": "comment", "value": "waypoint 2"},
    ],
  },
  "parentId": "953cb0f0-e587-4880-affe-90001da1262d",
}

newAreaPolygonPouint3Msg = {
  "@worldmodeltype": "RSGUpdate",
  "operation": "CREATE",
  "node": {
    "@graphtype": "Node",
    "id": "e2bb6580-a15f-4f90-84cc-a0696b031294", 
    "attributes": [
          {"key": "comment", "value": "waypoint 3"},
    ],
  },
  "parentId": "953cb0f0-e587-4880-affe-90001da1262d",
}

newAreaPolygonPouint4Msg = {
  "@worldmodeltype": "RSGUpdate",
  "operation": "CREATE",
  "node": {
    "@graphtype": "Node",
    "id": "bb54d653-4e8f-47a7-af9b-b0f935a181e2", 
    "attributes": [
          {"key": "comment", "value": "waypoint 4"},
    ],
  },
  "parentId": "953cb0f0-e587-4880-affe-90001da1262d",
}

# JSON messagea to CREATE a new Transforms between the GIS origin and the four polygon point. 
# Note that the parentId must exist beforehand, otherwise this operation does not succeed.
newTransformToPolygonPoint1Msg = {
  "@worldmodeltype": "RSGUpdate",
  "operation": "CREATE",
  "node": {    
    "@graphtype": "Connection",
    "@semanticContext":"Transform",
    "id": "114ecb5c-a716-4bd8-865b-734f1a4b538b",
    "attributes": [
      {"key": "tf:type", "value": "wgs84"}
    ],
    "sourceIds": [
      "953cb0f0-e587-4880-affe-90001da1262d",
    ],
    "targetIds": [
      "7a47e674-f3c3-47d9-aae3-fd558603076b",
    ],
    "history" : [
      {
        "stamp": {
          "@stamptype": "TimeStampDate",
          "stamp": "2016-05-31T09:20:42Z",
        },
        "transform": {
          "type": "HomogeneousMatrix44",
            "matrix": [
              [1,0,0,coodinates[0][0]],
              [0,1,0,coodinates[0][1]],
              [0,0,1,1500],
              [0,0,0,1] 
            ],
            "unit": "latlon"
        }
      }
    ], 	    
  },
  "parentId": "953cb0f0-e587-4880-affe-90001da1262d",
}

newTransformToPolygonPoint2Msg = {
  "@worldmodeltype": "RSGUpdate",
  "operation": "CREATE",
  "node": {    
    "@graphtype": "Connection",
    "@semanticContext":"Transform",
    "id": "0db85c28-ca1e-4659-afab-54e470a28448",
    "attributes": [
      {"key": "tf:type", "value": "wgs84"}
    ],
    "sourceIds": [
      "953cb0f0-e587-4880-affe-90001da1262d",
    ],
    "targetIds": [
      "2743bbaa-a590-42b7-aa38-d573c18fe6f6",
    ],
    "history" : [
      {
        "stamp": {
          "@stamptype": "TimeStampDate",
          "stamp": "2016-05-31T09:20:42Z",
        },
        "transform": {
          "type": "HomogeneousMatrix44",
            "matrix": [
              [1,0,0,coodinates[1][0]],
              [0,1,0,coodinates[1][1]],
              [0,0,1,1501],
              [0,0,0,1] 
            ],
            "unit": "latlon"
        }
      }
    ], 	    
  },
  "parentId": "953cb0f0-e587-4880-affe-90001da1262d",
}

newTransformToPolygonPoint3Msg = {
  "@worldmodeltype": "RSGUpdate",
  "operation": "CREATE",
  "node": {    
    "@graphtype": "Connection",
    "@semanticContext":"Transform",
    "id": "cc195472-cb50-49eb-becc-e0054da27b78",
    "attributes": [
      {"key": "tf:type", "value": "wgs84"}
    ],
    "sourceIds": [
      "953cb0f0-e587-4880-affe-90001da1262d",
    ],
    "targetIds": [
      "e2bb6580-a15f-4f90-84cc-a0696b031294",
    ],
    "history" : [
      {
        "stamp": {
          "@stamptype": "TimeStampDate",
          "stamp": "2016-05-31T09:20:42Z",
        },
        "transform": {
          "type": "HomogeneousMatrix44",
            "matrix": [
              [1,0,0,coodinates[2][0]],
              [0,1,0,coodinates[2][1]],
              [0,0,1,1502],
              [0,0,0,1] 
            ],
            "unit": "latlon"
        }
      }
    ], 	    
  },
  "parentId": "953cb0f0-e587-4880-affe-90001da1262d",
}

newTransformToPolygonPoint4Msg = {
  "@worldmodeltype": "RSGUpdate",
  "operation": "CREATE",
  "node": {    
    "@graphtype": "Connection",
    "@semanticContext":"Transform",
    "id": "3379294d-caa9-4a57-97e4-fa88de29b3bd",
    "attributes": [
      {"key": "tf:type", "value": "wgs84"}
    ],
    "sourceIds": [
      "953cb0f0-e587-4880-affe-90001da1262d",
    ],
    "targetIds": [
      "bb54d653-4e8f-47a7-af9b-b0f935a181e2",
    ],
    "history" : [
      {
        "stamp": {
          "@stamptype": "TimeStampDate",
          "stamp": "2016-05-31T09:20:42Z",
        },
        "transform": {
          "type": "HomogeneousMatrix44",
            "matrix": [
              [1,0,0,coodinates[3][0]],
              [0,1,0,coodinates[3][1]],
              [0,0,1,1503],
              [0,0,0,1] 
            ],
            "unit": "latlon"
        }
      }
    ], 	    
  },
  "parentId": "953cb0f0-e587-4880-affe-90001da1262d",
}

# JSON message to CREATE a new Connection that actually represents an area.
# It references all points that belong to a polygon describing the area in
# the targetIds field. Note, the first and last reference to a point have to be the 
# same to obtain a closed polygon.  
newAreaConnectionMsg = {
  "@worldmodeltype": "RSGUpdate",
  "operation": "CREATE",
  "node": {    
                                 
    "@graphtype": "Connection",
    "id": "8ce59f8e-6072-49c0-a0fc-481ee288e24b",
    "attributes": [
      {"key": "geo:area", "value": "polygon"},
    ],
    "sourceIds": [
    ],
    "targetIds": [
      "7a47e674-f3c3-47d9-aae3-fd558603076b",
      "2743bbaa-a590-42b7-aa38-d573c18fe6f6",
      "e2bb6580-a15f-4f90-84cc-a0696b031294",
      "bb54d653-4e8f-47a7-af9b-b0f935a181e2",
      "7a47e674-f3c3-47d9-aae3-fd558603076b",
    ],
    "start": { "@stamptype": "TimeStampUTCms" , "stamp": 0.0 },
    "end": { "@stamptype": "TimeStampDate" , "stamp": "2020-00-00T00:00:00Z" }	    
                              
  },
  "parentId": "e379121f-06c6-4e21-ae9d-ae78ec1986a1",
}


# Send the messages.
sendMessageToSWM(json.dumps(newOriginMsg))
sendMessageToSWM(json.dumps(newAreaPolygonPouint1Msg))
sendMessageToSWM(json.dumps(newTransformToPolygonPoint1Msg))
sendMessageToSWM(json.dumps(newAreaPolygonPouint2Msg))
sendMessageToSWM(json.dumps(newTransformToPolygonPoint2Msg))
sendMessageToSWM(json.dumps(newAreaPolygonPouint3Msg))
sendMessageToSWM(json.dumps(newTransformToPolygonPoint3Msg))
sendMessageToSWM(json.dumps(newAreaPolygonPouint4Msg))
sendMessageToSWM(json.dumps(newTransformToPolygonPoint4Msg))
sendMessageToSWM(json.dumps(newAreaConnectionMsg))

