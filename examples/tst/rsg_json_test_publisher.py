import zmq
import random
import sys
import time
import json

# The port is defined by the system composition (sherpa_world_model.usc) file
# via the ``local_json_in_port`` variable. 
# e.g.
# local local_json_in_port = "12911"  
#port = "12911"
port = "13911"  
if len(sys.argv) > 1:
    port =  sys.argv[1]
    int(port)

# Set up the ZMQ PUB-SUB communication layer.
context = zmq.Context()
socket = context.socket(zmq.PUB)
socket.bind("tcp://*:%s" % port)

# JSON message to CREATE a new Node. Note that the parentId must
# exist beforehands, otherwise this operation does not succeed.
# in this case the "parentId": "e379121f-06c6-4e21-ae9d-ae78ec1986a1"
# denotes the root id of the graph. It is set in the respective
# system composition (.usc) file and initialized the SHERPA
# world model with exactly that id.
#
# A Node can only be created once. Application of multiple 
# identical CREATE opration will be ignored (and reported via
# an appropriate warning). 
msg1 = {
  "@worldmodeltype": "RSGUpdate",
  "operation": "CREATE",
  "node": {
    "@graphtype": "Group",
    "id": "ff483c43-4a36-4197-be49-de829cdd66c8", 
    "attributes": [
          {"key": "name", "value": "Task Specification Tree"},
          {"key": "tst:envtst", "value": "{}"},
    ],
  },
  "parentId": "e379121f-06c6-4e21-ae9d-ae78ec1986a1",
}

# Same as above but with a complete TST as attribute with 
# key "tst:envtst". 
msg2 = {
  "@worldmodeltype": "RSGUpdate",
  "operation": "CREATE",
  "node": {
    "@graphtype": "Group",
    "id": "ff483c43-4a36-4197-be49-de829cdd66c8", 
    "attributes": [
          {"key": "name", "value": "Task Specification Tree"},
          {"key": "tst:envtst", "value": {
   "metamodel" : "sherpa_msgs",
   "model" : "",
   "payload" : {
      "tree" : {
         "approved" : 1,
         "children" : [
            {
               "csp_constraints" : [],
               "csp_variables" : [],
               "execunit" : "/uav0",
               "name" : "fly-to",
               "params" : {
                  "commanded-speed" : 3.0,
                  "delunit" : "",
                  "execunit" : "/uav0",
                  "execunitalias" : "",
                  "p" : {
                     "header" : {
                        "frame_id" : "world",
                        "rosype" : "Header",
                        "seq" : 0,
                        "stamp" : 1436106657.651721
                     },
                     "point" : {
                        "rosype" : "Point",
                        "x" : 30.0,
                        "y" : 4.0,
                        "z" : 1.0
                     },
                     "rosype" : "PointStamped"
                  },
                  "use-mp" : 0
               },
               "unique_node_id" : 2
            },
            {
               "csp_constraints" : [],
               "csp_variables" : [],
               "execunit" : "/uav0",
               "name" : "fly-to",
               "params" : {
                  "commanded-speed" : 3.0,
                  "delunit" : "",
                  "execunit" : "/uav0",
                  "execunitalias" : "",
                  "p" : {
                     "header" : {
                        "frame_id" : "world",
                        "rosype" : "Header",
                        "seq" : 0,
                        "stamp" : 1436106657.651853
                     },
                     "point" : {
                        "rosype" : "Point",
                        "x" : -30.0,
                        "y" : 2.0,
                        "z" : 1.0
                     },
                     "rosype" : "PointStamped"
                  },
                  "use-mp" : 0
               },
               "unique_node_id" : 3
            }
         ],
         "exec_info" : {
            "can_be_aborted" : 0,
            "can_be_paused" : 0
         },
         "exec_status" : {
            "aborted" : 0,
            "active" : 0,
            "executing" : 0,
            "finished" : 0,
            "paused" : 0,
            "succeeded" : 0,
            "waiting" : 0
         },
         "execunit" : "/uav0",
         "name" : "seq",
         "params" : {
            "execunit" : "/uav0",
            "execunitalias" : ""
         },
         "unique_node_id" : 1,
         "wait_for_etime" : 0,
         "wait_for_stime" : 0
      },
      "tree_id" : "uav0-1"
   },
   "type" : "execution-tst"
}
      },
    ],
  },
  "parentId": "e379121f-06c6-4e21-ae9d-ae78ec1986a1",
}

# JSON message to UPDATE Attributes of an already existing Node.
# The Node with id ff483c43-4a36-4197-be49-de829cdd66c8 has to 
# be created beforehands, otherwise the update can not be applied.
# All existing attributes will be completly overridden by this 
# update.
msg3 = {
  "@worldmodeltype": "RSGUpdate",
  "operation": "UPDATE_ATTRIBUTES",
  "node": {
    "@graphtype": "Group",
    "id": "ff483c43-4a36-4197-be49-de829cdd66c8", 
    "attributes": [
          {"key": "name", "value": "Task Specification Tree Update"},
          {"key": "tst:ennodeupdate", "value": {
   "metamodel" : "sherpa_msgs",
   "model" : "",
   "payload" : {
      "approved" : 1,
      "exec_info" : {
         "can_be_aborted" : 1,
         "can_be_paused" : 1
      },
      "exec_status" : {
         "aborted" : 0,
         "active" : 1,
         "executing" : 1,
         "finished" : 0,
         "paused" : 0,
         "succeeded" : 0,
         "waiting" : 0
      },
      "tree_id" : "uav0-1",
      "unique_node_id" : 2
   },
   "type" : "update-execution-tst-node"
}
      },
    ],
  },
}

# Same as above but the field "succeeded" is toggled to "1".
msg4 = {
  "@worldmodeltype": "RSGUpdate",
  "operation": "UPDATE_ATTRIBUTES",
  "node": {
    "@graphtype": "Group",
    "id": "ff483c43-4a36-4197-be49-de829cdd66c8", 
    "attributes": [
          {"key": "name", "value": "Task Specification Tree Update"},
          {"key": "tst:ennodeupdate", "value": {
   "metamodel" : "sherpa_msgs",
   "model" : "",
   "payload" : {
      "approved" : 1,
      "exec_info" : {
         "can_be_aborted" : 1,
         "can_be_paused" : 1
      },
      "exec_status" : {
         "aborted" : 0,
         "active" : 1,
         "executing" : 1,
         "finished" : 0,
         "paused" : 0,
         "succeeded" : 1,
         "waiting" : 0
      },
      "tree_id" : "uav0-1",
      "unique_node_id" : 2
   },
   "type" : "update-execution-tst-node"
}
      },
    ],
  },
}

# Pump out all the messages.
while True:
  print ("===")
  print (json.dumps(msg2))
  socket.send_string(json.dumps(msg2))   
  print ("---")
  print (json.dumps(msg3))
  socket.send_string(json.dumps(msg3))    
  print ("---")
  print (json.dumps(msg4))
  socket.send_string(json.dumps(msg4))
  time.sleep(1)

