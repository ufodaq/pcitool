import threading
import pcipywrap
import random
import os
import json
import requests
import time

class test_pcipywrap():
   def __init__(self, device, model, num_threads = 150,
   write_percentage = 0.1, register = 'test_prop2',
   server_host = 'http://localhost', server_port = 12412,
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
      self.pcilib = pcipywrap.Pcipywrap(device, model)
      self.num_threads = num_threads
      self.write_percentage = write_percentage
      self.register = register
      self.server_message_delay = server_message_delay
      self.server_port = server_port
      self.server_host = server_host
    
   def testThreadSafeReadWrite(self):
      def threadFunc():
         if random.randint(0, 100) >= (self.write_percentage * 100):
            ret = self.pcilib.get_property('/test/prop2')
            print self.register, ':', ret
            del ret
         else:
            val = random.randint(0, 65536)
            print 'set value:', val
            self.pcilib.write_register(val, self.register)
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
   		    #print self.pcilib.create_pcilib_instance('/dev/fpga0','test_pywrap')
   	  
            print self.pcilib.get_property_list('/test')
            print self.pcilib.get_register_info('test_prop1')
            #print self.pcilib.get_registers_list();
         
            #print self.pcilib.read_register('reg1')
            #print self.pcilib.write_register(12, 'reg1')
         
            #print self.pcilib.get_property('/test/prop2')
            #print self.pcilib.set_property(12, '/test/prop2')
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
      {'command': 'get_register_info', 'reg': 'reg1'},
      {'command': 'get_property_list'},
      {'command': 'read_register', 'reg': 'reg1'},
      {'command': 'write_register', 'reg': 'reg1'},
      {'command': 'get_property', 'prop': '/test/prop2'},
      {'command': 'set_property', 'prop': '/test/prop2'}]
      
      def sendRandomMessage():
         message_number = random.randint(1, len(payload) - 1)
         print 'message number: ', message_number
         payload[message_number]['value'] =  random.randint(0, 65535)
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
   lib = test_pcipywrap('/dev/fpga0','test_pywrap', num_threads = 150,
   write_percentage = 0.1, register = 'test_prop2',server_host = 'http://localhost', server_port = 12412,
   server_message_delay = 0)
   lib.testThreadSafeReadWrite()

