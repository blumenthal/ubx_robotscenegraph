#   Request-reply client in Python
#   Connects REQ socket to tcp://localhost:22422
#   Sends JSON Request as specifies by a file and
#   print the reply
#
import zmq
import sys

# Setup query
if len(sys.argv) > 1:
    fileName =  sys.argv[1]
    with open (fileName, "r") as messagefile:
      message=messagefile.read()
else:
    message="{}"

#  Prepare our context and sockets
context = zmq.Context()
socket = context.socket(zmq.REQ)
socket.connect("tcp://localhost:22422")
   
print("Sending query: %s " % (message))
socket.send_string(message)
result = socket.recv()
print("Received result: %s " % (result))

