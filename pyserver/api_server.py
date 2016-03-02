import os
import sys

import pcipywrap

import time
import json
from optparse import OptionParser

from multiprocessing import Process
from BaseHTTPServer import HTTPServer, BaseHTTPRequestHandler
from SocketServer import ThreadingMixIn

class MultiThreadedHTTPServer(ThreadingMixIn, HTTPServer):
    pass

class PcilibServerHandler(BaseHTTPRequestHandler):
   
   def __init__(s, pcilib, *args):
      s.pcilib = pcilib
      BaseHTTPRequestHandler.__init__(s, *args)
   
   def do_HEAD(s):
      s.send_response(200)
      s.send_header('content-type', 'application/json')
      s.end_headers()
      
   def do_GET(s):
      #run request in separate process
      p = Process(target=s.do_GET_worker, args=())
      p.start()
      p.join()
      
   def do_GET_worker(s):
      length = int(s.headers['Content-Length'])
      
      #deserialize input data
      data = json.loads(s.rfile.read(length).decode('utf-8'))
      
      if 'command' in data:
         command = data['command']
         if(command == 'help'):
            s.help(data)
            
            
            
         #elif(command == 'open'):
         #   #check required arguments
         #   if not 'device' in data:
         #      s.error('message doesnt contains "device" field, '
         #              'which is required for "open" command', data)
         #      return
         #   #parse command arguments and convert them to string
         #   device = str(data.get('device', None))
         #   model = data.get('model', None)
         #   if not model is None:
			#	model = str(model)
         #   
         #   try:
         #      s.openPcilibInstance(device, model)
         #   except Exception as e:
         #      s.error(str(e), data) 
         #     return
		   #
         #   #Success! Create and send reply
         #   out = dict()
         #   out['status'] = 'ok'
         #   s.wrapMessageAndSend(out, data)
            
            
            
         elif(command == 'get_registers_list'):
            #parse command arguments and convert them to string
            bank = data.get('bank', None)
            if not bank is None:
               bank = str(bank)

            registers = dict()
            try:
               registers = s.pcilib.get_registers_list(bank)
            except Exception as e:
               s.error(str(e), data) 
               return
               
            #Success! Create and send reply
            out = dict()
            out['status'] = 'ok'
            out['registers'] = registers
            s.wrapMessageAndSend(out, data)
          
          
          
         elif(command == 'get_register_info'):
            #check required arguments
            if not 'reg' in data:
               s.error('message doesnt contains "reg" field, '
                       'which is required for "get_register_info" command', data)
               return
               
            #parse command arguments and convert them to string
            reg = str(data.get('reg', None))
            bank = data.get('bank', None)
            if not bank is None:
               bank = str(bank)
            
            register = dict()
            try:
               register = s.pcilib.get_register_info(reg, bank)
            except Exception as e:
               s.error(str(e), data) 
               return
		
            #Success! Create and send reply
            s.wrapMessageAndSend({'status': 'ok', 'register': register}, data)
		 
       
       
         elif(command == 'get_property_list'):   
            #parse command arguments and convert them to string
            branch = data.get('branch', None)
            if not branch is None:
               branch = str(branch)
            
            properties = dict()
            try:
               properties = s.pcilib.get_property_list(branch)
            except Exception as e:
               s.error(str(e), data) 
               return
            	
            #Success! Create and send reply
            out = dict()
            out['status'] = 'ok'
            out['properties'] = properties
            s.wrapMessageAndSend(out, data)
            
            
            
         elif(command == 'read_register'):
            #check required arguments
            if not 'reg' in data:
               s.error('message doesnt contains "reg" field, '
                       'which is required for "read_register" command', data)
               return
               
            #parse command arguments and convert them to string
            reg = str(data.get('reg', None))
            bank = data.get('bank', None)
            if not bank is None:
				   bank = str(bank)
            
            value = 0
            try:
               value = s.pcilib.read_register(reg, bank)
            except Exception as e:
               s.error(str(e), data) 
               return
            
            #Success! Create and send reply
            out = dict()
            out['status'] = 'ok'
            out['value'] = value
            s.wrapMessageAndSend(out, data)
            
            
            
         elif(command == 'write_register'):
            #check required arguments
            if not 'reg' in data:
               s.error('message doesnt contains "reg" field, '
                       'which is required for "write_register" command', data)
               return
               
            if not 'value' in data:
               s.error('message doesnt contains "value" field, '
                       'which is required for "write_register" command', data)
               return
               
            #parse command arguments and convert them to string
            reg = str(data.get('reg', None))
            value = str(data.get('value', None))
            bank = data.get('bank', None)
            if not bank is None:
				   bank = str(bank)
            
            try:
               s.pcilib.write_register(value, reg, bank)
            except Exception as e:
               s.error(str(e), data) 
               return
            
            #Success! Create and send reply
            s.wrapMessageAndSend({'status': 'ok'}, data)
            
            
            
         elif(command == 'get_property'):
            #check required arguments
            if not 'prop' in data:
               s.error('message doesnt contains "prop" field, '
                       'which is required for "get_property" command', data)
               return
               
            #parse command arguments and convert them to string
            prop = str(data.get('prop', None))
            
            value = 0
            try:
               value = s.pcilib.get_property(prop)
            except Exception as e:
               s.error(str(e), data) 
               return
            
            #Success! Create and send reply
            out = dict()
            out['status'] = 'ok'
            out['value'] = value
            s.wrapMessageAndSend(out, data)
            
            
            
         elif(command == 'set_property'):
            #check required arguments
            if not 'prop' in data:
               s.error('message doesnt contains "prop" field, '
                       'which is required for "set_property" command', data)
               return
               
            if not 'value' in data:
               s.error('message doesnt contains "value" field, '
                       'which is required for "set_property" command', data)
               return
               
            #parse command arguments and convert them to string
            prop = str(data.get('prop', None))
            value = str(data.get('value', None))
            
            try:
               s.pcilib.set_property(value, prop)
            except Exception as e:
               s.error(str(e), data) 
               return
            
            #Success! Create and send reply
            s.wrapMessageAndSend({'status': 'ok'}, data)
            
            
            
         elif(command == 'lock'):
            #check required arguments
            if not 'lock_id' in data:
               s.error('message doesnt contains "lock_id" field, '
                       'which is required for "lock" command', data)
               return
               
            #parse command arguments and convert them to string
            lock_id = str(data.get('lock_id'))
            
            try:
               s.pcilib.lock(lock_id)
            except Exception as e:
               s.error(str(e), data) 
               return
            
            #Success! Create and send reply
            s.wrapMessageAndSend({'status': 'ok'}, data)
            
            
            
         elif(command == 'try_lock'):
            #check required arguments
            if not 'lock_id' in data:
               s.error('message doesnt contains "lock_id" field, '
                       'which is required for "try_lock" command', data)
               return
               
            #parse command arguments and convert them to string
            lock_id = str(data.get('lock_id'))
            
            try:
               s.pcilib.try_lock(lock_id)
            except Exception as e:
               s.error(str(e), data) 
               return
            
            #Success! Create and send reply
            s.wrapMessageAndSend({'status': 'ok'}, data)
            
            
            
         elif(command == 'unlock'):
            #check required arguments
            if not 'lock_id' in data:
               s.error('message doesnt contains "lock_id" field, '
                       'which is required for "unlock" command', data)
               return
               
            #parse command arguments and convert them to string
            lock_id = str(data.get('lock_id'))
            
            try:
               s.pcilib.unlock(lock_id)
            except Exception as e:
               s.error(str(e), data) 
               return

            #Success! Create and send reply
            s.wrapMessageAndSend({'status': 'ok'}, data)
            
            
            
         elif(command == 'get_scripts_list'):
            scripts = list()
            try:
               scripts = s.pcilib.get_scripts_list()
            except Exception as e:
               s.error(str(e), data) 
               return

            #Success! Create and send reply
            s.wrapMessageAndSend({'status': 'ok', 'scripts': scripts}, data)
         
         
         
         elif(command == 'run_script'):
            #check required arguments
            if not 'script_name' in data:
               s.error('message doesnt contains "script_name" field, '
                       'which is required for "run_script" command', data)
               return
            #parse command arguments and convert them to string
            script_name = str(data.get('script_name'))
            value = data.get('value', None)
            
            out = None
            try:
               out = s.pcilib.run_script(script_name, value)
            except Exception as e:
               s.error(str(e), data) 
               return

            #Success! Create and send reply
            if(type(out) == bytearray):
               s.send_response(200)
               s.send_header('content-disposition', 'inline; filename=value')
               s.send_header('content-type', 'application/octet-stream')
               s.end_headers()
               s.wfile.write(out)
            else:
               s.wrapMessageAndSend({'status': 'ok', 'value': out}, data)
            
            
            
         #elif(command == 'lock_global'):
         #   #check if global_lock already setted by server
         #   try:
         #      s.pcilib.lock_global()
         #   except Exception as e:
         #      s.error(str(e), data)
         #      return
         #   
         #   #Success! Create and send reply
         #   s.wrapMessageAndSend({'status': 'ok'}, data)
            
         
         
         #elif(command == 'unlock_global'):
         #   try:
         #      s.pcilib.unlock_global()
         #   except Exception as e:
         #      s.error(str(e), data)
         #      return
         #   
         #   #Success! Create and send reply
         #   s.wrapMessageAndSend({'status': 'ok'}, data)
         
         
         else:
		    s.error('command "' + command + '" undefined', data)
		    return
      else:
		  s.error('message doesnt contains "command" field, which is required', data)
		  return
        
        
      
   #open device context 
   #def openPcilibInstance(s, device, model):
   #   s.pcilib = pcipywrap.create_pcilib_instance(device, model)
         
   #Send help message
   def help(s, received_message = None):
      usage = str('Usage:\n'
      '  Server receive commands via http GET with json packet.\n'
      '  content-type should have value "application/json"\n'
      '  Server could handle only commands. to set command, you\n'
      '  should specify field "command" in packet with command name\n'
      '  List of commands:\n'
      '\n'
      '  command: help - Get help. This will return usage\n'
      '\n'
      
      '  command: open - Opens context of device. It will be reopened if already open.\n'
      '    required fields\n'
      '      device:       - path to the device file [/dev/fpga0]\n'
      '    optional fields\n'
      '      model:       - specifies the model of hardware, autodetected if doesnt exists\n'
      '\n'
      
      '  command: get_registers_list - Returns the list of registers provided by the hardware model.\n'
      '    optional fields\n'
      '      bank:        - if set, only register within the specified bank will be returned\n'
      '\n'
      
      '  command: get_register_info - Returns the information about the specified register.\n'
      '    required fields\n'
      '      reg:         - the name of the register\n'
      '    optional fields\n'
      '      bank:        - if set, only register within the specified bank will be returned\n'
      '\n'
      
      '  command: get_property_list - Returns the list of properties available under the specified path.\n'
      '    optional fields\n'
      '     branch:        - Path. If not set, will return the top-level properties\n'
      '\n'
      
      '  command: read_register - Reads the specified register.\n'
      '    required fields\n'
      '      reg:         - the name of the register\n'
      '    optional fields\n'
      '      bank:        - if set, only register within the specified bank will be processed\n'
      '\n'
      
      '  command: write_register - Writes to specified register.\n'
      '    required fields\n'
      '      reg:         - the name of the register\n'
      '      value:       - the register value to write. Should be int, float or string (with number)\n'
      '    optional fields\n'
      '      bank:        - if set, only register within the specified bank will be processed\n'
      '\n'
      
      '  command: get_property - Reads / computes the property value.\n'
      '    required fields\n'
      '      prop:         - full name including path\n'
      '\n'
      
      '  command: set_property - Writes the property value or executes the code associated with property.\n'
      '    required fields\n'
      '      prop:        - full name including path\n'
      '      value:       - the property value to write. Should be int, float or string (with number)\n'
      '\n'
      
      '  command: lock - function to acquire a lock, and wait till the lock can be acquire.\n'
      '    required fields\n'
      '      lock_id: - lock id\n'
      '\n'
      
      '  command: try_lock - this function will try to take a lock for the mutex pointed by \n'
      '                    lockfunction to acquire a lock, but that returns immediatly if the\n'
      '                    lock can\'t be acquired on first try\n'
      '      lock_id: - lock id\n'
      '\n'
      
      '  command: unlock - this function unlocks the lock.\n'
      '    required fields\n'
      '      lock_id: - lock id\n'
      '\n'
      
      '  command: get_scripts_list - Get aviable scripts with description\n'
      '\n'
      
      '  command: run_script - Run specified script\n'
      '    required fields\n'
      '      script_name: - script name (with extension)\n'
      '      value: - input value in json format\n'
      '\n'
      
      '\n')
      
      #send help as plain text
      s.send_response(200)
      s.send_header('content-type', 'text/plain')
      s.end_headers()
      s.wfile.write(usage)

   #Send error message with text description    
   def error(s, info, received_message = None):
      out = dict()
      
      out['status'] = 'error'
      out['description'] = info
      out['note'] = 'send {"command" : "help"} to get help'
      s.wrapMessageAndSend(out, received_message, 400)
        
   def wrapMessageAndSend(s, message, received_message = None, response = 200):
      s.send_response(response)
      s.send_header('content-type', 'application/json')
      s.end_headers()
      if not received_message is None:
         message['received_message'] = received_message
      s.wfile.write(json.dumps(message))

class ApiServer(MultiThreadedHTTPServer):
   def __init__(self, device='/dev/fpga0', model=None, adress=('0.0.0.0', 9000)):
      #redirect logs to exeption
      pcipywrap.redirect_logs_to_exeption()
      #pass Pcipywrap to to server handler
      self.lib = pcipywrap.Pcipywrap(device, model)
      def handler(*args):
         PcilibServerHandler(self.lib, *args)
      MultiThreadedHTTPServer.__init__(self, adress, handler)

if __name__ == '__main__':
   
   #parce command line options
   parser = OptionParser()
   parser.add_option("-p", "--port",  action="store",
                     type="int", dest="port", default=9000,
                     help="Set server port (9000)")
   parser.add_option("-d", "--device",  action="store",
                     type="string", dest="device", default=str('/dev/fpga0'),
                     help="FPGA device (/dev/fpga0)")                     
   parser.add_option("-m", "--model",  action="store",
                     type="string", dest="model", default=None,
                     help="Memory model (autodetected)")

   opts = parser.parse_args()[0]
   
   HOST_NAME = '0.0.0.0'
   PORT_NUMBER = opts.port
   MODEL = opts.model
   DEVICE = opts.device
   
   #start server
   httpd = ApiServer(DEVICE, MODEL, (HOST_NAME, PORT_NUMBER))
   
   print time.asctime(), "Server Starts - %s:%s" % (HOST_NAME, PORT_NUMBER)
   
   try:
      httpd.serve_forever()
   except KeyboardInterrupt:
      pass
      
   httpd.server_close()
   
   print time.asctime(), "Server Stops - %s:%s" % (HOST_NAME, PORT_NUMBER)
