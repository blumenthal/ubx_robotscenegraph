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
    
    def __init__(self, port, root_uuid, name, send_freq, max_vel, curr_pose, goal_pose):
        Thread.__init__(self)
        self.active = True
        
        #TODO: make stuff optional

        self.port = port #"22422"
        self.root_uuid = root_uuid #"853cb0f0-e587-4880-affe-90001da1262d"
        self.rob_name = name #"robot"
        self.send_freq = send_freq #10 #in Hz
        self.max_vel = max_vel # max velocity factor of robot
        self.current_pose = curr_pose
        self.goal_pose = goal_pose
        
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
        print "[{}]: Found a victim".format(self.rob_name)
        #TODO: createa apropriate msg
        
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
    rob1 = Sherpa_Actor("22422","853cb0f0-e587-4880-affe-90001da1262d","wasp1", 10, 0.001, current_pose, goal_pose)
    rob1.setName("wasp1")
    
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
    rob2 = Sherpa_Actor("22423","853cb0f0-e587-4880-affe-90001da1262d","GR", 10, 0.0001, current_pose, goal_pose)
    rob2.setName("GR")
    
    
    # start robots activity
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
    rob1.shutdown()
    rob2.shutdown()

    
    # wait for threads to finish
    rob1.join(30)
    rob2.join(30)
    
    print("Shutting down")
