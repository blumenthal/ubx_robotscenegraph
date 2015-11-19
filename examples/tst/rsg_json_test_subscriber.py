import sys
import zmq

#port = "12911"
port = "12912"
if len(sys.argv) > 1:
    port =  sys.argv[1]
    int(port)
    
if len(sys.argv) > 2:
    port1 =  sys.argv[2]
    int(port1)

# Socket to talk to server
context = zmq.Context()
socket = context.socket(zmq.SUB)

print ("Collecting TST updates from world model agent...")
socket.connect ("tcp://localhost:%s" % port)

if len(sys.argv) > 2:
    socket.connect ("tcp://localhost:%s" % port1)

topicfilter = ""
if sys.version_info < (3,):
  socket.setsockopt_string(zmq.SUBSCRIBE, topicfilter.decode('utf_8')) # Python2
else:
  socket.setsockopt_string(zmq.SUBSCRIBE, topicfilter) # Python3

while True:
  string = socket.recv()
  print (string)

