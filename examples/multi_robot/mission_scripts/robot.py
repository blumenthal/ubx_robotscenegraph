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

uav_update_port=12921 # cf. SWM_LOCAL_JSON_IN_PORT for robot #2

class Sherpa_Actor(Thread):
    """ The Sherpa Actor Class"""
    
    def __init__(self):
        Thread.__init__(self)
        self.active = True
        self.port = uav_update_port
        self.root_uuid = "853cb0f0-e587-4880-affe-90001da1262d"
        self.rob_name = "robot"
        self.send_freq = 10 #in Hz
        self.max_vel = 0.001 # max velocity factor of robot
        self.current_pose = [
                        [1,0,0,45.84561555807046],
                      [0,1,0,7.72886713924574],
                      [0,0,1,3.0],
                      [0,0,0,1] 
                    ]
        self.goal_pose = [
                      [1,0,0,50.0],
                      [0,1,0,16.0],
                      [0,0,1,5.0],
                      [0,0,0,1] 
                    ]
        
        self.origin_uuid = str(uuid.uuid4())
        self.rob_uuid = str(uuid.uuid4())
        self.tranform_origin_rob_uuid = str(uuid.uuid4())
        
        # Set up the ZMQ PUB-SUB communication layer.
        self.context = zmq.Context()
        self.socket = self.context.socket(zmq.PUB)
        self.socket.bind("tcp://*:%s" % self.port)
        
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
          "parentId": self.root_uuid,
        }
        self.socket.send_string(json.dumps(newTransformMsg)) 
        #print (json.dumps(newTransformMsg))
 
    def shutdown(self):
        self.active = False
        print "["+self.rob_name+"]: Received shut down"
        
    def set_goal(self,new_goal):
        # check of new_goal is a 4x4 matrix
        if (not len(new_goal) == 4) or (not len(new_goal[0]) == 4):
            print "["+self.rob_name+"]: new goal is not a 4x4 matrix"
            return
        # should also check if proper matrix
        self.goal_pose = new_goal
        print "["+self.rob_name+"]: Changed goal pose to"
        print self.goal_pose
        
    def victim_found(self):
        print "["+self.rob_name+"]: Found a victim"
        #TODO: createa apropriate msg

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
                                          "@stamptype": "TimeStampDate",
                                          "stamp": datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S'),
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
            print "position"
            print "[{},{},{}]".format(self.current_pose[0][3],self.current_pose[1][3],self.current_pose[2][3])
            print "#####"
            time.sleep(1/self.send_freq)

if __name__ == '__main__':
    
    # create robot in its own thread
    rob1 = Sherpa_Actor()
    rob1.setName("rob1")
    # start its activity
    rob1.start()
    
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
    rob1.shutdown()

    
    # wait for threads to finish
    rob1.join()
    
    print("Shutting down")
