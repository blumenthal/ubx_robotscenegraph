#   Request-reply client in Python
#   Uses Zyre via the libswmzyre as comminucation backend.
#   Sends JSON Request as specifies by the first argument,
#   waits for a reply and print the reply message.
#

import sys
import time
import datetime
import ctypes
import json


# Setup messge
if len(sys.argv) > 1:
    fileName =  sys.argv[1]
    with open (fileName, "r") as messagefile:
      message=messagefile.read()
else:
    message="{}"
    print("No parameters specified.")
    print("Usage:")
    print("\t python ./" + sys.argv[0] + " <path_to_json_file> [<path_to_config_file>]")
    print("Example:")
    print("\t python ./" + sys.argv[0] + " new_node.json ../zyre/swm_zyre_donkey.json")
    exit()

# Setup config file
configFile='../zyre/swm_zyre_config.json'
if len(sys.argv) > 2:
    configFile =  sys.argv[2]

### Setup Zyre helper library: swmzyrelib
swmzyrelib = ctypes.CDLL('../zyre/build/libswmzyre.so')
swmzyrelib.wait_for_reply.restype = ctypes.c_char_p
cfg = swmzyrelib.load_config_file(configFile)
component = swmzyrelib.new_component(cfg)

### Define helper method to send a single messagne and receive its reply 
def sendZyreMessageToSWM(message):
  print("Sending message: %s " % (message))
  jsonMsg = swmzyrelib.encode_json_message_from_string(component, message);
  err = swmzyrelib.shout_message(component, jsonMsg);
  timeOutInMilliSec = 5000
  result = swmzyrelib.wait_for_reply(component, jsonMsg, timeOutInMilliSec);
  print("Received result: %s " % ( json.dumps(json.loads(result),indent=2, sort_keys=False ) ))
  
  return result

# Send message
sendZyreMessageToSWM(message)

# Clean up
#swmzyrelib.destroy_component(ctypes.addressof(component));
