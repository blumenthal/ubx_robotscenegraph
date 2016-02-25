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

# A priori knowledge from secene_setup() - we also might query for it,
# but this is not necessary here.
swm_ugv_update_port=12911 # cf. SWM_LOCAL_JSON_IN_PORT for robot #1
swm_uav_update_port=12921 # cf. SWM_LOCAL_JSON_IN_PORT for robot #2
swm_hmi_update_port=12931 # cf. SWM_LOCAL_JSON_IN_PORT for robot #3

# Groups
swm_root_uuid =   "e379121f-06c6-4e21-ae9d-ae78ec1986a1"
swm_robots_uuid = "fe234f0a-8742-4506-ad35-8974f02f848c"
swm_victims_uuid = "42b8ea33-50de-41e2-b7ee-51a966b379e9"
swm_origin_uuid = "853cb0f0-e587-4880-affe-90001da1262d"
swm_ugv_uuid =    "bc31c854-fd29-4987-87d5-8e81cd94a4a9"
swm_uav_uuid =    "e8685dab-74d3-4ada-b969-8c1caa017598"
swm_hmi_uuid =    "bfc0e783-7266-44b0-9f3d-b2c40db87987"


# Transforms
swm_ugv_tf_uuid = "f25a2385-e6ec-4324-8061-90e92cba0c00"
swm_uav_tf_uuid = "d01d79b5-ea24-49fc-a1cb-feb6d29ebe77"
swm_hmi_tf_uuid = ""

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
        
        # Set up the ZMQ PUB-SUB communication layer.
        self.context = zmq.Context()
        self.socket = self.context.socket(zmq.PUB)
        self.socket.bind("tcp://*:%s" % self.port)
        time.sleep(1)        

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
          self.socket.send_string(json.dumps(newOriginMsg))
          
          # create robot node
          newNodeMsg = {
            "@worldmodeltype": "RSGUpdate",
            "operation": "CREATE",
            "node": {
              "@graphtype": "Node",
              "id": self.rob_uuid, 
              "attributes": [
                    {"key": "name", "value": self.rob_name},
              ],
            },
            "parentId": self.root_uuid,
          }
          self.socket.send_string(json.dumps(newNodeMsg))  
          
          # create transform for initial condition of robot
          newTransformMsg = {
            "@worldmodeltype": "RSGUpdate",
            "operation": "CREATE",
            "node": {
              "@graphtype": "Connection",
              "@semanticContext":"Transform",
              "id": self.tranform_origin_rob_uuid,
              "attributes": [
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
          self.socket.send_string(json.dumps(newTransformMsg)) 
          print (json.dumps(newTransformMsg))
 
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
        time.sleep(0.1)
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
        time.sleep(0.1)
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
            self.socket.send_string(json.dumps(transforUpdateMsg))
            print "[{}]: position".format(self.name)
            print "[{},{},{}]".format(self.current_pose[0][3],self.current_pose[1][3],self.current_pose[2][3])
            print "#####"
            time.sleep(1/self.send_freq)

if __name__ == '__main__':
    
    # create robot in its own thread
    #Sherpa_Actor(port,root_uuid, name, send_freq, max_vel, current_pose, goal_pose)
    current_pose = [
                [1,0,0,45.84561555807046],
                [0,1,0,7.72886713924574],
                [0,0,1,3.0],
                [0,0,0,1] 
                ]
    goal_pose = [
                [1,0,0,50],
                [0,1,0,15],
                [0,0,1,8.0],
                [0,0,0,1] 
                ]
    rob1 = Sherpa_Actor(True, swm_uav_update_port,swm_origin_uuid,"uav", 1, 0.001, current_pose, goal_pose, swm_origin_uuid, swm_uav_uuid, swm_uav_tf_uuid, swm_victims_uuid)
    rob1.setName("uav")
    
    current_pose = [
                [1,0,0,45.84561555807046],
                [0,1,0,7.72886713924574],
                [0,0,1,1.0],
                [0,0,0,1] 
                ]
    goal_pose = [
                [1,0,0,40],
                [0,1,0,5],
                [0,0,1,1.0],
                [0,0,0,1] 
                ]
    rob2 = Sherpa_Actor(True, swm_ugv_update_port,swm_origin_uuid,"ugv", 1, 0.0001, current_pose, goal_pose, swm_origin_uuid, swm_ugv_uuid, swm_ugv_tf_uuid, swm_victims_uuid)
    rob2.setName("ugv")
    
    
    # start robots activity
    #rob1.victim_found()
    rob1.start()
    rob2.start()
    
    t0 = time.time()
    while (time.time()-t0 < 5):
        print time.clock()
        time.sleep(0.5)
    rob1.set_goal([
              [1,0,0,35.0],
              [0,1,0,5.0],
              [0,0,1,4.0],
              [0,0,0,1] 
              ])      
    t0 = time.time()
    while (time.time()-t0 < 5):
        print time.clock()
        time.sleep(0.5)
    rob1.victim_found()
    time.sleep(1)
    rob1.shutdown()
    rob2.shutdown()

    
    # wait for threads to finish
    rob1.join(30)
    rob2.join(30)
    
    print("Shutting down")
