#   Request-reply client in Python
#   Uses Zyre via the libswmzyre as comminucation backend.
#   Sends JSON Request as specifies by the first argument,
#   waits for a reply and print the reply message.
#

import sys
import time
import datetime
import ctypes

### Setup Zyre helper library: swmzyrelib
swmzyrelib = ctypes.CDLL('../zyre/build/libswmzyre.so')
swmzyrelib.wait_for_reply.restype = ctypes.c_char_p
cfg = swmzyrelib.load_config_file('../zyre/swm_zyre_config.json')
component = swmzyrelib.new_component(cfg)

### Define helper method to send a single messagne and receive its reply 
def sendZyreMessageToSWM(message):
  print("Sending message: %s " % (message))
  jsonMsg = swmzyrelib.encode_json_message_from_string(component, message);
  err = swmzyrelib.shout_message(component, jsonMsg);
  result = swmzyrelib.wait_for_reply(component);
  print("Received result: %s " % (result))
  return result


# Setup query
if len(sys.argv) > 1:
    fileName =  sys.argv[1]
    with open (fileName, "r") as messagefile:
      message=messagefile.read()
else:
    message="{}"

# Send message
sendZyreMessageToSWM(message)

# Clean up
#swmzyrelib.destroy_component(ctypes.addressof(component));
