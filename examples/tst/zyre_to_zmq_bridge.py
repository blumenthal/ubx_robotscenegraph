
from pyre import Pyre 

#from pyre import zhelper 
import zmq 
import uuid
import logging
import time
import json

# No validation used for now
#from jsonspec.validators import load


'''
'''
JSONEncoder_olddefault = json.JSONEncoder.default
def JSONEncoder_newdefault(self, o):
    if isinstance(o, uuid.UUID): return str(o)
    return JSONEncoder_olddefault(self, o)
json.JSONEncoder.default = JSONEncoder_newdefault



class ZyreToZMQModule:
    """ The ZyreToZMQModule Base Class """
   
    
    def __init__(self,ctx = None,name = "Default",headers=None,groups=None,config_file=None,polling_timeout = 1,sim_time=2):

        self.logger = logging.getLogger(name)
        self.logger.setLevel(logging.DEBUG)
        # create the console output logging
        sh = logging.StreamHandler()
        sh.setLevel(logging.INFO)
        # create a logger writing into a log file
        fh = logging.FileHandler(name + ".log")
        fh.setLevel(logging.DEBUG)
        # create formatter and add it to the handlers
        # see https://docs.python.org/2/library/logging.html#logrecord-attributes for what can be logged
        fh.setFormatter(logging.Formatter('Name: %(name)s, ProcessName: %(processName)s, ThreadName: %(threadName)s, Module: %(module)s, FunctionName: %(funcName)s, LineNo: %(lineno)d, Level: %(levelname)s, Msg: %(message)s'))
        #sh.setFormatter(logging.Formatter('Module: %(module)s, FunctionName: %(funcName)s, LineNo: %(lineno)d, Level: %(levelname)s, Msg: %(message)s'))
        sh.setFormatter(logging.Formatter('Name: %(name)s, %(module)s/%(funcName)s[%(levelname)s]: %(message)s (lineno:%(lineno)d)'))
        # add the handlers to logger
        self.logger.addHandler(sh)
        self.logger.addHandler(fh)
        self.logger.propagate = False # don't pass messages to parent loggers

        if not type(ctx) ==  zmq.sugar.context.Context:
            self.logger.warn("Created ZMQ context. Please supply a ZMQ context next time.")
            ctx = zmq.Context()
        self.ctx = ctx
        if not type(name) == str:
            name = "Default"
            self.logger.warn("Please provide a the name as a string. Setting name to default: %s." % name)
        self.name = name
        if not headers:
            self.logger.warn("Please provide a model and other header information.")
            headers = {}
        self.headers = headers
        if not groups:
            groups = ["SHERPA"]
            self.logger.warn("Please provide a list with group names to which it will listen. Will set it to default: %s" %groups)
        self.groups = groups
        if not config_file:
            # will be opened during configuration
            self.logger.error("No config file provided for initial configuration")
            exit()
      
        self.config_file = config_file
        self.config = None
        self.alive = True # used for exiting once the node is stopped
        self.line_controller = None # stores uid of line controller this module is listening to (to avoid several line controllers)
        self.com_timeout = polling_timeout # time_out in msec after which poller returns
        self.sim_time = sim_time # time in msec
        self.inbox = []
        self.outbox = []
             
        # Node setup
        self.node = Pyre(self.name)
        for key in self.headers:
            self.node.set_header(key,self.headers[key])
        for group in self.groups:
            self.node.join(group)
        #self.node.set_verbose()
        self.node.start()
        
        # Socket setup
        self.poller = zmq.Poller()
        self.poller.register(self.node.socket(), zmq.POLLIN)

        port = "12911" 
        self.zmq_socket = self.ctx.socket(zmq.PUB)
        self.zmq_socket.bind("tcp://*:%s" % port)
   

        self.run()


    def run(self):
        """ Running loop of this process:
        [COMMUNICATION]     Check communication channels and parse all messages.
        """
        while self.alive:
            try:
                #[COMMUNICATION]
                self.communication()
                #[LOGGING]
                #self.logging()
                time.sleep(1)
            except (KeyboardInterrupt, SystemExit):
                break
        self.node.stop()


    def communication(self):
        """ This function handles the communication """

        # Check if communication is ready
        if not self.poller:
            self.logger.warn("No communication channels yet.")
        # Process input
        items = dict(self.poller.poll(self.com_timeout))
        # Get input from Pyre socket and put it in inbox which is processed in the following steps
        #logger.debug("Start processing incoming messages")
        if self.node.socket() in items and items[self.node.socket()] == zmq.POLLIN: 
            self.logger.info("Received %d msgs." % len(items))
            #print "msg arrived. inbox len %d" % len(self.inbox)
            msg_frame = self.node.recv()
            msg_type = msg_frame.pop(0)
            peer_uid = uuid.UUID(bytes=msg_frame.pop(0))
            peer_name = msg_frame.pop(0)   
            self.logger.info("Msg type: %s" % msg_type)         
            self.logger.info("Sender: %s" % peer_name)   
            # For shout and whisper take message payload and put it into inbox assuming it is correct
            # For the other zyre msgs create a JSON obj and put that into inbox
            # TODO: add validation
            if msg_type == "SHOUT":
                group = msg_frame.pop(0)
                msg = json.loads(msg_frame.pop(0))
                msg["sender_UUID"] = peer_uid
                msg["sender_name"] = peer_name
#                self.inbox.append(msg)
                self.outbox.append(msg)
            elif msg_type == "WHISPER":
                msg = json.loads(msg_frame.pop(0))
                msg["sender_UUID"] = peer_uid
                msg["sender_name"] = peer_name
                self.inbox.append(msg)        
            # don't care about join and leave messages; if necessary, split to have separate events               
#             elif msg_type == "LEAVE" or msg_type == "JOIN":
#                 group = msg_frame.pop(0)
#                 event = json.dumps({"metamodel": "http://www.sherpa-project.eu/examples/sherpa_msgs",
#                                     "model": "http://www.sherpa-project.eu/examples/sherpa_msgs/zyre_events",
#                                     "type": "leave/join",
#                                     "sender_UUID": peer_uid,
#                                     "sender_name": peer_name,
#                                     "payload": {}
#                                     })
#                 self.inbox.append(event)
            elif msg_type == "ENTER":
                headers = json.loads(msg_frame.pop(0))
                event = {"metamodel": "http://www.sherpa-project.eu/examples/sherpa_msgs",
                         "model": "http://www.sherpa-project.eu/examples/sherpa_msgs/zyre_events",
                         "type": "enter",
                         "sender_UUID": peer_uid,
                         "sender_name": peer_name,
                         "payload": {"header": headers}
                         }
                self.inbox.append(event)
            elif msg_type == "EXIT":
                event = {"metamodel": "http://www.sherpa-project.eu/examples/sherpa_msgs",
                         "model": "http://www.sherpa-project.eu/examples/sherpa_msgs/zyre_events",
                         "type": "exit",
                         "sender_UUID": peer_uid,
                         "sender_name": peer_name,
                         "payload": {}
                         }
                self.inbox.append(event)
        # Process output and send all msgs there until it is empty, but only if a line controller is connected.
        if self.outbox: #only do if there really is something in the outbox
            #if self.lcsm.sm.curr_state._name == "Idle" or self.lcsm.sm.curr_state._name == "Busy":
            if true:
                self.logger.info("Start processing messages in outbox. Number of msgs in outbox: %d" % len(self.outbox))
                while self.outbox:
                    # since we assume all msgs conform to the sherpa_msgs JSON model, simply shout them out
                    # TODO: add validation
                    msg = self.outbox.pop()
                    if type(msg) == dict:
                        self.logger.info("outbox msg type: " + msg["type"])
                        self.node.shout("SHERPA", json.dumps(msg))
                        
                        #WM message
                        
                        msg_wm_update = {
                          "@worldmodeltype": "RSGUpdate",
                          "operation": "CREATE",
                          "node": {
                            "@graphtype": "Group",
                            "id": "ff483c43-4a36-4197-be49-de829cdd66c8", 
                            "attributes": [
                                  {"key": "name", "value": "Task Specification Tree"},
                                  {"key": "tst:envtst", "value": msg },
                            ],
                          },
                          "parentId": "e379121f-06c6-4e21-ae9d-ae78ec1986a1",
                        }
                  
                        print ("===")
                        print (json.dumps(msg_wm_update))
                        self.zmq_socket.send_string(json.dumps(msg_wm_update)) 

                    else:
                        self.logger.warn("Message is not a dict! Serialization is done here! Msg: \n" + str(msg))
            else:
                self.logger.info("Can't send messages in outbox. Module not ready.")
                     
               
        

        

if __name__ == '__main__':    
    ctx = zmq.Context()
    headers = {"MODEL":"http://people.mech.kuleuven.be/~u0097847/SHERPA/json/feature_schema.json",
               "TYPE":"WASP_0"
               }
    groups = ["SHERPA"]
    config_file = "models/tf_config.json"
    tf = ZyreToZMQModule(ctx,"ZyreToZMQModule",headers,groups,config_file)
   
    print("ZyreToZMQModule shut down.")

