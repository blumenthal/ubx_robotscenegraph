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
x=45.8450
y=7.73180
z=0

initialVoltage = 15.3 #V

i=0
updateFreq = 1 # Hz e.g. 10
slowUpdateRatio = 5 # updateFreq/slowUptateRatio => freq for slow update; e.g. 10 is 10th
verySlowUpdateRatio = 60 # 60 once per minute in case of updateFreq = 10

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

# Add an agent. The method check automatocally if it exists already.


matrix = MATRIX4x4(1, 0, 0, 0,
			             0, 1, 0, 0,
			             0, 0, 1, 0,
			             x, y, z, 1)
stamp = DOUBLE(time.time()*1000.0)
result = swmzyrelib.add_agent(component, matrix, stamp, agentName)
print("Result for add_agent: %s " % (result))

if result == 1:

  # Continously update data
  while (True):

    i=i+1 # generic counter 

    z=i
    matrix = MATRIX4x4(1, 0, 0, 0,
 			               0, 1, 0, 0,
			               0, 0, 1, 0,
			               x, y, z, 1)

    batteryVoltage = initialVoltage - 0.01 * i


    stamp1 = time.time()*1000.0


  
    #pose
    result = swmzyrelib.update_pose(component, matrix, DOUBLE(stamp1), agentName)


    #artva
    if ((i % slowUpdateRatio) == 0):
      print("UPDATE ARTVA")
      artva = ARTVA(4104+i,0,0,0,i,0,0,0)
      result = swmzyrelib.add_artva_measurement(component, artva, agentName)


      print("UPDATE BATTERY")
      result = swmzyrelib.add_battery(component, DOUBLE(batteryVoltage), "HIGH", agentName)


    if ((i % verySlowUpdateRatio) == 0):
      print("UPDATE IMAGE")
      result = swmzyrelib.add_image(component, matrix, DOUBLE(stamp1), agentName, "/tmp/image00" + str(i) + ".jpg")

      print("UPDATE VICTIM")
      result = swmzyrelib.add_victim(component, matrix, DOUBLE(stamp1), agentName)

    stamp2 = time.time()*1000.0 
    stamp3 = (stamp2-stamp1)/1000.0
    print("%f UPDATE TOOK %f [s]" % (stamp1, stamp3) )
    time.sleep(1.0/updateFreq)

else:
  print("ERROR: Agent could not be added.")



