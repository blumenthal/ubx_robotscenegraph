# Example on how to continously puplish pose data using the convenience library 
# and Python

import zmq
import random
import sys
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



##############################################################

# Variables used for this example
x=45.8450
y=7.73180
z=0

updateFreq = 5 # Hz

# Types to be used with wrapper
DOUBLE = ctypes.c_double
MATRIX4x4 = DOUBLE * 16

##############################################################

# Add an agent. The method check automatocally if it exists already.


matrix = MATRIX4x4(1, 0, 0, 0,
			             0, 1, 0, 0,
			             0, 0, 1, 0,
			             x, y, z, 1)
stamp = DOUBLE(time.time()*1000.0)
result = swmzyrelib.add_agent(component, matrix, stamp, agentName)
print("Result for add_agent: %s " % (result))

if result == 1:

  # Continously updata data
  while (True):

    z=z+1
    matrix = MATRIX4x4(1, 0, 0, 0,
 			               0, 1, 0, 0,
			               0, 0, 1, 0,
			               x, y, z, 1)

    stamp = DOUBLE(time.time()*1000.0)
    result = swmzyrelib.update_pose(component, matrix, stamp, agentName)
    time.sleep(1.0/updateFreq)

else:
  print("ERROR: Agent could not be added.")



