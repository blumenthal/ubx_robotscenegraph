# Example for a robot that receives a goal_pose and flies there in a straight line.
# 
from threading import Thread
import zmq
import sys
import time
import datetime
import json
import uuid
import math
import sys
import swm


# A priori knowledge from secene_setup() - we also might query for it,
# but this is not necessary here.
#swm_ugv_update_port=12911 # cf. SWM_LOCAL_JSON_IN_PORT for robot #1
swm_uav_update_port=12911 # cf. SWM_LOCAL_JSON_IN_PORT for robot #2
#swm_hmi_update_port=12931 # cf. SWM_LOCAL_JSON_IN_PORT for robot #3

# Groups
swm_root_uuid =   "e379121f-06c6-4e21-ae9d-ae78ec1986a1"
swm_robots_uuid = "fe234f0a-8742-4506-ad35-8974f02f848c"
swm_victims_uuid = "42b8ea33-50de-41e2-b7ee-51a966b379e9"
swm_origin_uuid = "853cb0f0-e587-4880-affe-90001da1262d"
swm_ugv_uuid =    "bc31c854-fd29-4987-87d5-8e81cd94a4a9"
swm_uav_uuid =    "e8685dab-74d3-4ada-b969-8c1caa017598"
swm_hmi_uuid =    "bfc0e783-7266-44b0-9f3d-b2c40db87987"


# Transforms
#swm_ugv_tf_uuid = "f25a2385-e6ec-4324-8061-90e92cba0c00"
swm_uav_tf_uuid = "d01d79b5-ea24-49fc-a1cb-feb6d29ebe77"
#swm_hmi_tf_uuid = ""

# Comment on timeing precision
# Either (JSON parser resolves with second resolution)
#"stamp": {
#  "@stamptype": "TimeStampDate",
#  "stamp": datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S.%f')[:-3]
#},
#
#"stamp": {
#  "@stamptype": "TimeStampUTCms",
#  "stamp": time.time()*1000.0
#},




class Sherpa_Actor(Thread):
    """ The Sherpa Actor Class"""
    
    def __init__(self, scene_setup_is_done = False, port = "22422", root_uuid = None, name = "robot", send_freq = 10, max_vel = 0.001, curr_pose = None, goal_pose = None, origin_uuid = None , rob_uuid = None, tranform_origin_rob_uuid = None, detected_victim_group_uuid = None):
        Thread.__init__(self)
        self.active = True
        self.scene_setup_is_done = scene_setup_is_done

        self.port = port #"22422"
        if root_uuid:
            self.root_uuid = root_uuid #"853cb0f0-e587-4880-affe-90001da1262d"
        else:
            print "create random root_uuid"
            self.root_uuid = str(uuid.uuid4())
        self.rob_name = name #"robot"
        self.send_freq = send_freq #10 #in Hz
        self.max_vel = max_vel # max velocity factor of robot

        # for battery simulation
        self.battery_uuid = "052d2a58-9566-46d7-adef-336f280baf2d" #str(uuid.uuid4())
        # A wasp has a voltage range of 13.5V  16.8V
        self.battery_min = 13.5 # [V]
        self.battery_max = 16.8 # [V]
        self.battery_discharge_per_second = 0.1 # [V/s] 
        self.battery_voltage = self.battery_max          

        # artva simulation 
        self.artva_uuid = "9218643f-a71d-48bc-8cf1-8a701bd5a5fd" #str(uuid.uuid4())
        self.artva_signal = 0 # range [0-100] ?!?
        self.artva_range = 0.0001 # sort of lat lon distance ....

        if curr_pose:
            self.current_pose = curr_pose
        else:
            print "set current pose to review setting at Champoluc"
            self.current_pose = [
                [1,0,0,45.84561555807046],
                [0,1,0,7.72886713924574],
                [0,0,1,3.0],
                [0,0,0,1] 
                ]
        if goal_pose:
            self.goal_pose = goal_pose
        else:
            print "setting goal_pose to current_pose"
            self.goal_pose = self.current_pose   
        if origin_uuid:
            self.origin_uuid = origin_uuid 
        else:
            print "create random origin_uuid"
            self.origin_uuid = str(uuid.uuid4())
        if rob_uuid:
            self.rob_uuid = rob_uuid 
        else:
            print "create random rob_uuid"
            self.rob_uuid = str(uuid.uuid4())
        if tranform_origin_rob_uuid:
            self.tranform_origin_rob_uuid = tranform_origin_rob_uuid 
        else:
            print "create random tranform_origin_rob_uuid"
            self.tranform_origin_rob_uuid = str(uuid.uuid4())
        if detected_victim_group_uuid:
            self.detected_victim_group_uuid = detected_victim_group_uuid
        else:
            print "create random detected_victim_group_uuid"
            self.detected_victim_group_uuid = str(uuid.uuid4())

        self.victim_pose = self.goal_pose       
        
#        # Set up the ZMQ PUB-SUB communication layer.
#        self.context = zmq.Context()
#        self.socket = self.context.socket(zmq.PUB)
#        self.socket.bind("tcp://*:%s" % self.port)
#        time.sleep(1)        

        if(self.scene_setup_is_done == False):
          # create origin node as subgroup of root
          newOriginMsg = {
            "@worldmodeltype": "RSGUpdate",
            "operation": "CREATE",
            "node": {
              "@graphtype": "Node",
              "id": self.origin_uuid,
              "attributes": [
                {"key": "gis:origin", "value": "wgs84"},
                {"key": "comment", "value": "Reference frame for geo poses. Use this ID for Transform queries."},
              ],
          
            },
            "parentId": self.root_uuid,
          }
#          self.socket.send_string(json.dumps(newOriginMsg))
          print (json.dumps(newOriginMsg))
          swm.updateSWM(newOriginMsg)          

          # create robot node
          newNodeMsg = {
            "@worldmodeltype": "RSGUpdate",
            "operation": "CREATE",
            "node": {
              "@graphtype": "Group",
              "id": self.rob_uuid, 
              "attributes": [
                    {"key": "shepa:agent_name", "value": self.rob_name},
              ],
            },
            "parentId": self.root_uuid,
          }
#          self.socket.send_string(json.dumps(newNodeMsg))  
          print (json.dumps(newNodeMsg))
          swm.updateSWM(newNodeMsg)            

          # create transform for initial condition of robot
          newTransformMsg = {
            "@worldmodeltype": "RSGUpdate",
            "operation": "CREATE",
            "node": {
              "@graphtype": "Connection",
              "@semanticContext":"Transform",
              "id": self.tranform_origin_rob_uuid,
              "attributes": [
                {"key": "name", "value": self.rob_name + "_geopose"},
	    	        {"key": "tf:type", "value": "wgs84"}
              ],
              "sourceIds": [
                self.origin_uuid,
              ],
              "targetIds": [
                self.rob_uuid,
              ],
              "history" : [
                {
                  "stamp": {
                    "@stamptype": "TimeStampDate",
                    "stamp": datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S') + "Z"
                  },
                  "transform": {
                    "type": "HomogeneousMatrix44",
                      "matrix": self.current_pose,
                      "unit": "latlon"
                  }
                }
              ], 	    
            },
            "parentId": self.root_uuid,
          }
#          self.socket.send_string(json.dumps(newTransformMsg)) 
          swm.updateSWM(newTransformMsg)  
          print (json.dumps(newTransformMsg))

          # create node to represent the battery state
          newBatteryMsg = {
            "@worldmodeltype": "RSGUpdate",
            "operation": "CREATE",
            "node": {
              "@graphtype": "Node",
              "id": self.battery_uuid, 
              "attributes": [
                    {"key": "sensor:battery", "value": self.battery_voltage},
              ],
            },
            "parentId": self.rob_uuid,
          }
          swm.updateSWM(newBatteryMsg)  
          print (json.dumps(newBatteryMsg))

          # sensor node for ARTVA?
          newArtvaMsg = {
            "@worldmodeltype": "RSGUpdate",
            "operation": "CREATE",
            "node": {
              "@graphtype": "Node",
              "id": self.artva_uuid, 
              "attributes": [
                    {"key": "sherpa:artva_signal", "value": self.artva_signal},
              ],
            },
            "parentId": self.rob_uuid,
          }
          swm.updateSWM(newArtvaMsg)  
          print (json.dumps(newArtvaMsg)) 

    def shutdown(self):
        self.active = False
        print "[{}]: Received shut down".format(self.rob_name)
        
    def set_goal(self,new_goal):
        # check of new_goal is a 4x4 matrix
        if (not len(new_goal) == 4) or (not len(new_goal[0]) == 4):
            print "[{}]: new goal is not a 4x4 matrix".format(self.rob_name)
            return
        # should also check if proper matrix
        self.goal_pose = new_goal
        print "[{}]: Changed goal pose to".format(self.rob_name)
        print self.goal_pose

        goal_uuid = str(uuid.uuid4())
        newGoalMsg = {
          "@worldmodeltype": "RSGUpdate",
          "operation": "CREATE",
          "node": {
            "@graphtype": "Node",
            "id": goal_uuid, 
            "attributes": [
                  {"key": "sar:area", "value": "center"},
            ],
          },
          "parentId": "1cd7e823-5b01-44cf-ad5a-572c2c3dbf96",
        }
        self.socket.send_string(json.dumps(newGoalMsg)) 
        print (json.dumps(newGoalMsg))
        newGaolTfMsg = {
            "@worldmodeltype": "RSGUpdate",
            "operation": "CREATE",
            "parentId": self.origin_uuid,
            "node": {
              "@graphtype": "Connection",
              "@semanticContext":"Transform",
              "id": str(uuid.uuid4()),
              "attributes": [
                {"key": "tf:type", "value": "wgs84"}
              ],
              "sourceIds": [
                self.origin_uuid,
              ],
              "targetIds": [
                goal_uuid,
              ],
              "history" : [
                {
                  "stamp": {
                    "@stamptype": "TimeStampDate",
                    "stamp": datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S')
                  },
                  "transform": {
                    "type": "HomogeneousMatrix44",
                      "matrix": self.goal_pose,
                      "unit": "latlon"
                  }
                }
              ],         
            },
        }
        self.socket.send_string(json.dumps(newGaolTfMsg))
        
    def victim_found(self):
        victim_uuid = str(uuid.uuid4())
        newNodeMsg = {
          "@worldmodeltype": "RSGUpdate",
          "operation": "CREATE",
          "node": {
            "@graphtype": "Node",
            "id": victim_uuid, 
            "attributes": [
                  {"key": "sar:detection", "value": "victim"},
                  {"key": "sar:clothing_color", "value": "yellow"}
            ],
          },
          "parentId": self.detected_victim_group_uuid,
        }
        self.socket.send_string(json.dumps(newNodeMsg)) 
        print (json.dumps(newNodeMsg))
        #time.sleep(0.1)
        newVicFoundMsg = {
            "@worldmodeltype": "RSGUpdate",
            "operation": "CREATE",
            "parentId": self.origin_uuid,
            "node": {
              "@graphtype": "Connection",
              "@semanticContext":"Transform",
              "id": str(uuid.uuid4()),
              "attributes": [
                {"key": "tf:type", "value": "wgs84"}
              ],
              "sourceIds": [
                self.origin_uuid,
              ],
              "targetIds": [
                victim_uuid,
              ],
              "history" : [
                {
                  "stamp": {
                    "@stamptype": "TimeStampDate",
                    "stamp": datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S')
                  },
                  "transform": {
                    "type": "HomogeneousMatrix44",
                      "matrix": self.current_pose,
                      "unit": "latlon"
                  }
                }
              ],         
            },
        }
        self.socket.send_string(json.dumps(newVicFoundMsg))
        #time.sleep(0.1)
        print (json.dumps(newVicFoundMsg))
        print "[{}]: Found a victim".format(self.rob_name)
        
        
    def set_send_freq(self,freq):
        if (not isinstance( freq, ( int, long ) ) ):
            print "[{}]: Frequency is not an integer".format(self.rob_name)
            return
        self.send_freq = freq
        print "[{}]: changed send_frequency to {}".format(self.rob_name, freq)
        
    def set_max_vel(self,max_vel):
        if (not isinstance( max_vel, float ) ):
            print "[{}]: Max velocity is not a float".format(self.rob_name)
            return
        self.max_vel = max_vel
        print "[{}]: changed max velocity to {}".format(self.rob_name, max_vel)
        
    def run(self):
        while (self.active):
            # calculate difference between current and goal pos
            lat_diff = self.goal_pose[0][3] - self.current_pose[0][3]
            long_diff = self.goal_pose[1][3] - self.current_pose[1][3]
            alt_diff = self.goal_pose[2][3] - self.current_pose[2][3]
            
            # calculate length and limit to max_vel
            length = math.sqrt(pow(lat_diff, 2)+pow(long_diff, 2)+pow(alt_diff, 2))
            if (length > self.max_vel):
                lat_diff /= length
                lat_diff *= self.max_vel
                long_diff /= length
                long_diff *= self.max_vel
                alt_diff /= length
                alt_diff *= self.max_vel
            self.current_pose[0][3] += lat_diff
            self.current_pose[1][3] += long_diff
            self.current_pose[2][3] += alt_diff
             
            # create and send update message
            transforUpdateMsg = {                      
                                  "@worldmodeltype": "RSGUpdate",
                                  "operation": "UPDATE_TRANSFORM",
                                  "node": {    
                                    "@graphtype": "Connection",
                                    "@semanticContext":"Transform",
                                    "id": self.tranform_origin_rob_uuid,
                                    "history" : [
                                      {
                                        "stamp": {
                                          "@stamptype": "TimeStampUTCms",
                                          "stamp": time.time()*1000.0
                                        },
                                        "transform": {
                                          "type": "HomogeneousMatrix44",
                                          "matrix": self.current_pose,
                                          "unit": "latlon"
                                        }
                                      }
                                    ],
                                  }
                                }
 
            # simulate battery discharge
            cycle_time_in_seconds = 1.0/self.send_freq # [s]
            print ("cycle_time_in_seconds: " + str(cycle_time_in_seconds))
            self.battery_voltage =  self.battery_voltage - cycle_time_in_seconds * self.battery_discharge_per_second
            if self.battery_voltage < self.battery_min:
              self.battery_voltage = self.battery_min

            batteryUpdateMsg = {
              "@worldmodeltype": "RSGUpdate",
              "operation": "UPDATE_ATTRIBUTES",
              "node": {
                "@graphtype": "Node",
                "id": self.battery_uuid,
                "attributes": [
                      {"key": "sensor:battery_voltage", "value": self.battery_voltage},
                ],         
               },
            }


            # simulate ARTVA based on distance              
            # calculate difference between current and goal pos
            lat_diff = self.victim_pose[0][3] - self.current_pose[0][3]
            long_diff = self.victim_pose[1][3] - self.current_pose[1][3]
            alt_diff = self.victim_pose[2][3] - self.current_pose[2][3]
            
            # calculate length 
            length = math.sqrt(pow(lat_diff, 2)+pow(long_diff, 2))
            print "distance to artva = {}".format(length)
            if length < self.artva_range:
              self.artva_signal = 100 - (length * self.artva_range/100.0) #fixme                          
            else:
              self.artva_signal = 0

            artvaUpdateMsg = {
              "@worldmodeltype": "RSGUpdate",
              "operation": "UPDATE_ATTRIBUTES",
              "node": {
                "@graphtype": "Node",
                "id": self.artva_uuid,
                "attributes": [
                    {"key": "sherpa:artva_signal", "value": self.artva_signal}
                ],         
               },
            }

            # fire and forget version (not yet working with zyre)            
            swm.pubSWM(transforUpdateMsg)
            swm.pubSWM(batteryUpdateMsg)
            swm.pubSWM(artvaUpdateMsg)

            # version with ACK maessage
#            swm.updateSWM(transforUpdateMsg) 

           

            print "[{}]: position".format(self.name)
            print "[{},{},{}]".format(self.current_pose[0][3],self.current_pose[1][3],self.current_pose[2][3])
            print "battery level: [{} V]".format(self.battery_voltage)
            print "artva signal: [{} ]".format(self.artva_signal)
            print "#####"
            time.sleep(1.0/self.send_freq)

if __name__ == '__main__':
    
    # 1.) python swm.py intialize
    swm.run("initialize")

    # 2.) load_map

    # Get correct ids 15142
    swm.initCom()    

    objectAttribute = "gis:origin"
    objectValue = "wgs84"
    getOrigin = {
      "@worldmodeltype": "RSGQuery",
      "query": "GET_NODES",
       "attributes": [
        {"key": objectAttribute, "value": objectValue },
      ]
    }      
    result = swm.query(getOrigin)
    swm_origin_uuid = result["ids"][0]
    print("swm_origin_uuid = " + swm_origin_uuid)

    objectAttribute = "name"
    objectValue = "animals"
    getAnimalsGroup = {
      "@worldmodeltype": "RSGQuery",
      "query": "GET_NODES",
       "attributes": [
        {"key": objectAttribute, "value": objectValue },
      ]
    }      
    result = swm.query(getAnimalsGroup)
    swm_animals_uuid = result["ids"][0]
    print("swm_animals_uuid = " + swm_animals_uuid)

    objectAttribute = "name"
    objectValue = "swm"
    getMissionGroup = {
      "@worldmodeltype": "RSGQuery",
      "query": "GET_NODES",
       "attributes": [
        {"key": objectAttribute, "value": objectValue },
      ]
    }      
    result = swm.query(getMissionGroup)
    swm_mission_uuid = result["ids"][0]
    print("swm_mission_uuid = " + swm_mission_uuid)

    # Add fixed mission frame at: 9.61712430331891 40.38291805158355 
    fixedMssionFramePos = [ 9.61712430331891, 40.38291805158355 ]
    missionOriginId = "e3c33a96-9923-4eeb-9a9b-e1b57ce67f86"    
    newMissionOriginMsg = {
      "@worldmodeltype": "RSGUpdate",
      "operation": "CREATE",
      "node": {
        "@graphtype": "Node",
        "id": missionOriginId,
        "attributes": [
          {"key": "sherpa:origin", "value": "initial"},
          {"key": "comment", "value": "Reference frame for the mission. Typcally an initial odometry frame of one of the animals."},
         ],          
       },
     "parentId": swm_mission_uuid,
    }
    print (json.dumps(newMissionOriginMsg))
    swm.updateSWM(newMissionOriginMsg)          

    missionOriginGeoposeId = "86f210f0-f52a-40d6-95d0-d5f9254fb476"    
    newMissionOriginMsg = {
      "@worldmodeltype": "RSGUpdate",
      "operation": "CREATE",
      "node": {
        "@graphtype": "Connection",
        "@semanticContext":"Transform",
        "id": str(uuid.uuid4()),
        "attributes": [
	        {"key": "tf:type", "value": "wgs84"}
        ],
        "sourceIds": [
          swm_origin_uuid,
        ],
        "targetIds": [
          missionOriginId,
        ],
        "history" : [
           {
            "stamp": {
              "@stamptype": "TimeStampDate",
               "stamp": datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S') + "Z"
            },
            "transform": {
              "type": "HomogeneousMatrix44",
	               "matrix": [
                    [1,0,0,fixedMssionFramePos[0]],
                    [0,1,0,fixedMssionFramePos[1]],
                    [0,0,1,0.0],
                    [0,0,0,1] 
	                ],
                 "unit": "latlon"
             }
          }
        ], 	    
     },
     "parentId": swm_origin_uuid,
    }

    print (json.dumps(newMissionOriginMsg))
    swm.updateSWM(newMissionOriginMsg)    


    # create robot in its own thread
    #Sherpa_Actor(port,root_uuid, name, send_freq, max_vel, current_pose, goal_pose

    waspInitialPos = [ 9.617424, 40.382180 ]
    waspWayPoint1 =  [ 9.617428, 40.382176 ]
    waspWayPoint2 =  [ 9.617687, 40.382690 ]
    victimPos =      [ 9.617667, 40.383019 ] 

    current_pose = [
                [1,0,0,waspInitialPos[0]],
                [0,1,0,waspInitialPos[1]],
                [0,0,1,3.0],
                [0,0,0,1] 
                ]
    goal_pose = [
                [1,0,0,waspWayPoint1[0]],
                [0,1,0,waspWayPoint1[1]],
                [0,0,1,8.0],
                [0,0,0,1] 
                ]
    rob1 = Sherpa_Actor(False, swm_uav_update_port,swm_animals_uuid,"wasp_1", 100, 0.05, current_pose, goal_pose, swm_origin_uuid, swm_uav_uuid, swm_uav_tf_uuid, swm_victims_uuid)
    rob1.setName("wasp_1")
    #exit(0)    

    # start robots activity
    rob1.start()
  
    t0 = time.time()
    while (time.time()-t0 < 5):
        print time.clock()
        time.sleep(0.5)
    rob1.set_goal([
              [1,0,0,waspWayPoint2[0]],
              [0,1,0,waspWayPoint2[1]],
              [0,0,1,10.0],
              [0,0,0,1] 
              ])      
    t0 = time.time()
    while (time.time()-t0 < 5):
        print time.clock()
        time.sleep(0.5)
    #rob1.victim_found()
    time.sleep(2)
    rob1.shutdown()

    
    # wait for threads to finish
    rob1.join(30)
    
    print("Shutting down")
