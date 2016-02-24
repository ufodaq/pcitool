import threading
import pcipywrap
import random
import os
import json
import requests
import time
from optparse import OptionParser, OptionGroup

class test_pcipywrap():
   def __init__(self, 
                device, 
                model, 
                num_threads = 150,
                write_percentage = 0.1, 
                register = 'reg1',
                prop = '/test/prop1',
                branch = '/test',
                server_host = 'http://localhost', 
                server_port = 12412,
                server_message_delay = 0):
	  #initialize enviroment variables
      if not 'APP_PATH' in os.environ:
         APP_PATH = ''
         file_dir = os.path.dirname(os.path.abspath(__file__))
         APP_PATH = str(os.path.abspath(file_dir + '/../..'))
         os.environ["APP_PATH"] = APP_PATH
       
      if not 'PCILIB_MODEL_DIR' in os.environ:   
         os.environ['PCILIB_MODEL_DIR'] = os.environ["APP_PATH"] + "/xml"
      if not 'LD_LIBRARY_PATH' in os.environ: 
         os.environ['LD_LIBRARY_PATH'] = os.environ["APP_PATH"] + "/pcilib"
   
      random.seed()
      #create pcilib_instance
      self.device = device
      self.model = model
      self.pcilib = pcipywrap.Pcipywrap(device, model)
      self.num_threads = num_threads
      self.write_percentage = write_percentage
      self.register = register
      self.prop = prop
      self.branch = branch
      self.server_message_delay = server_message_delay
      self.server_port = server_port
      self.server_host = server_host

   def testLocking(self, message, lock_id = None):
      url = str(self.server_host + ':' + str(self.server_port))
      headers = {'content-type': 'application/json'}
      payload =[{'command': 'lock', 'lock_id': lock_id},
                {'command': 'try_lock', 'lock_id': lock_id},
                {'command': 'unlock', 'lock_id': lock_id},
                {'command': 'lock_global'},
                {'command': 'unlock_global'},
                {'command': 'help'}]  
      r = requests.get(url, data=json.dumps(payload[message]), headers=headers)
      print json.dumps(r.json(), sort_keys=True, indent=3, separators=(',', ': '))
    
   def testThreadSafeReadWrite(self):
      def threadFunc():
         if random.randint(0, 100) >= (self.write_percentage * 100):
            ret = self.pcilib.get_property(self.prop)
            print self.register, ':', ret
            del ret
         else:
            val = random.randint(0, 65536)
            print 'set value:', val
            self.pcilib.set_property(val, self.prop)
      try:
         while(1):
            thread_list = [threading.Thread(target=threadFunc) for i in range(0, self.num_threads)]
            for i in range(0, self.num_threads):
               thread_list[i].start()
            for i in range(0, self.num_threads):
               thread_list[i].join()
            print 'cycle done'
      except KeyboardInterrupt:
         print 'testing done'
         pass
   
   def testMemoryLeak(self):
      try:
         while(1):
            val = random.randint(0, 8096)
            self.pcilib = pcipywrap.Pcipywrap(self.device, self.model)
            print self.pcilib.get_property_list(self.branch)
            print self.pcilib.get_register_info(self.register)
            print self.pcilib.get_registers_list();
         
            print self.pcilib.read_register(self.register)
            print self.pcilib.write_register(val, self.register)
         
            print self.pcilib.get_property(self.prop)
            print self.pcilib.set_property(val, self.prop)
      except KeyboardInterrupt:
         print 'testing done'
         pass

   def testServer(self):
      url = str(self.server_host + ':' + str(self.server_port))
      headers = {'content-type': 'application/json'}
      payload =[{'com': 'open', 'data2' : '12341'},
      #{'command': 'open', 'device' : '/dev/fpga0', 'model': 'test_pywrap'},
      {'command': 'help'},
      {'command': 'get_registers_list'},
      {'command': 'get_register_info', 'reg': self.register},
      {'command': 'get_property_list', 'branch': self.branch},
      {'command': 'read_register', 'reg': self.register},
      {'command': 'write_register', 'reg': self.register},
      {'command': 'get_property', 'prop': self.prop},
      {'command': 'set_property', 'prop': self.prop}]
      
      def sendRandomMessage():
         message_number = random.randint(1, len(payload) - 1)
         print 'message number: ', message_number
         payload[message_number]['value'] =  random.randint(0, 8096)
         r = requests.get(url, data=json.dumps(payload[message_number]), headers=headers)
         print json.dumps(r.json(), sort_keys=True, indent=4, separators=(',', ': '))
      
      try:    
         r = requests.get(url, data=json.dumps(payload[1]), headers=headers)
         print json.dumps(r.json(), sort_keys=True, indent=3, separators=(',', ': '))
   
         while(1):
            time.sleep(self.server_message_delay)
            thread_list = [threading.Thread(target=sendRandomMessage) for i in range(0, self.num_threads)]
            for i in range(0, self.num_threads):
               thread_list[i].start()
            for i in range(0, self.num_threads):
               thread_list[i].join()
            print 'cycle done'
            
      except KeyboardInterrupt:
         print 'testing done'
         pass

if __name__ == '__main__':
   #parce command line options
   parser = OptionParser()
   parser.add_option("-d", "--device",  action="store",
                     type="string", dest="device", default=str('/dev/fpga0'),
                     help="FPGA device (/dev/fpga0)")                     
   parser.add_option("-m", "--model",  action="store",
                     type="string", dest="model", default=None,
                     help="Memory model (autodetected)")
   parser.add_option("-t", "--threads",  action="store",
                     type="int", dest="threads", default=150,
                     help="Threads number (150)")
                     
   server_group = OptionGroup(parser, "Server Options",
                              "Options for testing server.")
   server_group.add_option("-p", "--port",  action="store",
                           type="int", dest="port", default=9000,
                           help="Set testing server port (9000)")
   server_group.add_option("--host",  action="store",
                           type="string", dest="host", default='http://localhost',
                           help="Set testing server host (http://localhost)")
   server_group.add_option("--delay",  action="store",
                           type="float", dest="delay", default=0.0,
                           help="Set delay in seconds between sending messages to setver (0.0)")
   parser.add_option_group(server_group)
   
   rw_group = OptionGroup(parser, "Registers/properties Options",
                         "Group stores testing register and property options")
   rw_group.add_option("--write_percentage",  action="store",
                       type="float", dest="write_percentage", default=0.5,
                       help="Set percentage (0.0 - 1.0) of write commands in multithread (0.5)"
                       "read/write test")
   rw_group.add_option("-r", "--register", action="store",
                       type="string", dest="register", default='reg1',
                       help="Set register name (reg1)")
   rw_group.add_option("--property", action="store",
                       type="string", dest="prop", default='/test/prop1',
                       help="Set property name (/test/prop1)")
   rw_group.add_option("-b", "--branch", action="store",
                       type="string", dest="branch", default='/test',
                       help="Set property branch (/test)")
                       
   parser.add_option_group(rw_group)
   
   test_group = OptionGroup(parser, "Test commands group",
                            "This group conatains aviable commands for testing. "
                            "If user add more than one command, they will process"
                            "sequientally. To stop test, press Ctrl-C."
                            )
   test_group.add_option("--test_mt_rw",  action="store_true",
                       dest="test_mt_rw", default=False,
                       help="Multithread read/write test. This test will execute "
                       "get_property/set_property commands with random values in multi-thread mode")
   test_group.add_option("--test_memory_leak",  action="store_true",
                       dest="test_memory_leak", default=False,
                       help="Python wrap memory leak test. This test will execute all"
                       "Python wrap memory leak test. This test will execute all"
                       )
   test_group.add_option("--test_server",  action="store_true",
                       dest="test_server", default=False,
                       help="Python server test. This test will send "
                       "random commands to server in multi-thread mode"
                       )
   parser.add_option_group(test_group)
   
   lock_group = OptionGroup(parser, "Locking test group")
   
   lock_group.add_option("--lock",  action="store",
                       dest="lock_id", default=None, type="string",
                       help="Lock id."
                       )
   lock_group.add_option("--try_lock",  action="store",
                       dest="try_lock_id", default=None, type="string",
                       help="Try to lock id."
                       )
   lock_group.add_option("--unlock",  action="store",
                       dest="unlock_id", default=None, type="string",
                       help="Unlock lock with id."
                       )
   lock_group.add_option("--lock_global",  action="store_true",
                       dest="lock_global", default=False,
                       help="Global pcilib lock."
                       )
   lock_group.add_option("--unlock_global",  action="store_true",
                       dest="unlock_global", default=False,
                       help="Global pcilib unlock."
                       )
                       
   lock_group.add_option("--get_server_message",  action="store_true",
                       dest="get_server_message", default=False,
                       help="Global pcilib unlock."
                       )
                       

   parser.add_option_group(lock_group)
   
   opts = parser.parse_args()[0]

   #create pcilib test instance
   lib = test_pcipywrap(opts.device,
                        opts.model,
                        num_threads = opts.threads,
                        write_percentage = opts.write_percentage,
                        register = opts.register,
                        prop = opts.prop,
                        branch = opts.branch,
                        server_host = opts.host,
                        server_port = opts.port,
                        server_message_delay = opts.delay)
   
   #make some tests                
   if opts.test_mt_rw:
      lib.testThreadSafeReadWrite()
   if opts.test_memory_leak:
      lib.testMemoryLeak()
   if opts.test_server:
      lib.testServer()
   if opts.lock_id:
      lib.testLocking(0, opts.lock_id)  
   if opts.try_lock_id:
      lib.testLocking(1, opts.try_lock_id)  
   if opts.unlock_id:
      lib.testLocking(2, opts.unlock_id)  
   if opts.lock_global:
      lib.testLocking(3)  
   if opts.unlock_global:
      lib.testLocking(4)  
   if(opts.get_server_message):
      lib.testLocking(5)  

