# Example on how to continously puplish pose data using the convenience library 
# and Python

import zmq
import random
import sys
import signal
import time
import json
import uuid
import datetime
import ctypes
from array import array

###################### Config ##################################

# Setup config file and agent name
configFile='../zyre/swm_zyre_config.json'
agentName="fw0"

if len(sys.argv) == 3:
    agentName =  sys.argv[1]
    configFile = sys.argv[2]


else:
    message="{}"
    print("No parameters specified.")
    print("Usage:")
    print("\t python ./" + sys.argv[0] + " agent_name <path_to_config_file>")
    print("Example:")
    print("\t python ./" + sys.argv[0] + " fw0 ../zyre/swm_zyre_donkey.json")
    exit()



###################### Zyre ##################################


### Setup Zyre helper library: swmzyrelib
swmzyrelib = ctypes.CDLL('../zyre/build/libswmzyre.so')
swmzyrelib.wait_for_reply.restype = ctypes.c_char_p
cfg = swmzyrelib.load_config_file(configFile)
component = swmzyrelib.new_component(cfg)
timeOutInMilliSec = 5000

### Define helper method to send a single messagne and receive its reply 
def sendZyreMessageToSWM(message):
  print("Sending message: %s " % (message))
  jsonMsg = swmzyrelib.encode_json_message_from_string(component, message);
  err = swmzyrelib.shout_message(component, jsonMsg);
  result = swmzyrelib.wait_for_reply(component, jsonMsg, timeOutInMilliSec);
  #result = swmzyrelib.wait_for_reply(component);
  print("Received result: %s " % (result))
  return result

def signal_handler(signal, frame):
        print('You pressed Ctrl+C Stopping.')
        time.sleep(1)
        swmzyrelib.destroy_component(component)
        sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

##############################################################

# Variables used for this example
x=0
y=0
z=0

i=0
updateFreq = 2 # Hz e.g. 10

# Types to be used with wrapper
DOUBLE = ctypes.c_double
MATRIX4x4 = DOUBLE * 16

#typedef struct _artva_measurement_t {
#	int signal0;
#	int signal1;
#	int signal2;
#	int signal3;
#	int angle0;
#	int angle1;
#	int angle2;
#	int angle3;
#} artva_measurement;

class ARTVA(ctypes.Structure):
  _fields_ = [("signal0", ctypes.c_int),
              ("signal1", ctypes.c_int),
              ("signal2", ctypes.c_int),
              ("signal3", ctypes.c_int),
              ("angle0", ctypes.c_int),
              ("angle1", ctypes.c_int),
              ("angle2", ctypes.c_int), 
              ("angle3", ctypes.c_int)]

##############################################################

# Continously query data
while (True):

  i=i+1 # generic counter 

  matrix = MATRIX4x4(1, 0, 0, 0,
 			               0, 1, 0, 0,
			               0, 0, 1, 0,
			               x, y, z, 1)
  stamp1 = time.time()*1000.0
  
  #pose
  result = swmzyrelib.get_pose(component, matrix, DOUBLE(stamp1), agentName)

  if result == 1:
    print("STATUS [ " + agentName + " ]: (" + str(matrix[12]) + ", " + str(matrix[13]) + ", " + str(matrix[14]) + ")" )
  else:
    print("STATUS [ " + agentName + " ]: WARNIG: Agent does not exist.")    

  stamp2 = time.time()*1000.0 
  stamp3 = (stamp2-stamp1)/1000.0
  print("UPDATE TOOK %f [s]" % stamp3 )
  time.sleep(1.0/updateFreq)

