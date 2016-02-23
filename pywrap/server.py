import time
import os
import pcipywrap
import json
import BaseHTTPServer
import sys
from optparse import OptionParser

class PcilibServerHandler(BaseHTTPServer.BaseHTTPRequestHandler):
   def __init__(s, pcilib, *args):
      s.pcilib = pcilib
      BaseHTTPServer.BaseHTTPRequestHandler.__init__(s, *args)
   
   def do_HEAD(s):
      s.send_response(200)
      s.send_header('content-type', 'application/json')
      s.end_headers()
      
   def do_GET(s):
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
         #   s.send_response(200)
         #   s.send_header('content-type', 'application/json')
         #   s.end_headers()
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
            s.send_response(200)
            s.send_header('content-type', 'application/json')
            s.end_headers()
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
            s.send_response(200)
            s.send_header('content-type', 'application/json')
            s.end_headers()
            out = dict()
            out['status'] = 'ok'
            out['register'] = register
            s.wrapMessageAndSend(out, data)
		 
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
            s.send_response(200)
            s.send_header('content-type', 'application/json')
            s.end_headers()
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
            s.send_response(200)
            s.send_header('content-type', 'application/json')
            s.end_headers()
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
            s.send_response(200)
            s.send_header('content-type', 'application/json')
            s.end_headers()
            out = dict()
            out['status'] = 'ok'
            s.wrapMessageAndSend(out, data)
            
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
            s.send_response(200)
            s.send_header('content-type', 'application/json')
            s.end_headers()
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
            s.send_response(200)
            s.send_header('content-type', 'application/json')
            s.end_headers()
            out = dict()
            out['status'] = 'ok'
            s.wrapMessageAndSend(out, data)
            
            	
         else:
		    s.error('command "' + command + '" undefined', data)
		    return
      else:
		  s.error('message doesnt contains "command" field, which is required', data)
		  return
		  
       
      #print str(s.headers['content-type'])
      #print post_data['some']
      
   """open device context """
   def openPcilibInstance(s, device, model):
      s.pcilib = pcipywrap.create_pcilib_instance(device, model)
         
   """Send help message"""
   def help(s, received_message = None):
      s.send_response(200)
      s.send_header('content-type', 'application/json')
      s.end_headers()
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
      '\n')
      out = {'status': 'ok', 'usage' : usage}
      s.wrapMessageAndSend(out, received_message)

   """Send error message with text description"""     
   def error(s, info, received_message = None):
      s.send_response(400)
      s.send_header('content-type', 'application/json')
      s.end_headers()
      out = dict()
      
      out['status'] = 'error'
      out['description'] = info
      out['note'] = 'send {"command" : "help"} to get help'
      s.wrapMessageAndSend(out, received_message)
        
   def wrapMessageAndSend(s, message, received_message = None):
      if not received_message is None:
         message['received_message'] = received_message
      s.wfile.write(json.dumps(message))

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
   
   HOST_NAME = ''
   PORT_NUMBER = opts.port
   MODEL = opts.model
   DEVICE = opts.device
   
   #Set enviroment variables, if it not setted already
   if not 'APP_PATH' in os.environ:
      APP_PATH = ''
      file_dir = os.path.dirname(os.path.abspath(__file__))
      APP_PATH = str(os.path.abspath(file_dir + '/../..'))
      os.environ["APP_PATH"] = APP_PATH

   if not 'PCILIB_MODEL_DIR' in os.environ:   
      os.environ['PCILIB_MODEL_DIR'] = os.environ["APP_PATH"] + "/xml"
      
   if not 'LD_LIBRARY_PATH' in os.environ: 
      os.environ['LD_LIBRARY_PATH'] = os.environ["APP_PATH"] + "/pcilib"
   
   #redirect logs to exeption
   pcipywrap.__redirect_logs_to_exeption()
   
   #start server
   pcilib_server = BaseHTTPServer.HTTPServer
   
   #pass Pcipywrap to to server handler
   lib = pcipywrap.Pcipywrap(DEVICE, MODEL)
   def handler(*args):
      PcilibServerHandler(lib, *args)
   
   httpd = pcilib_server((HOST_NAME, PORT_NUMBER), handler)
   
   print time.asctime(), "Server Starts - %s:%s" % (HOST_NAME, PORT_NUMBER)
   try:
      httpd.serve_forever()
   except KeyboardInterrupt:
      pass
   httpd.server_close()
   print time.asctime(), "Server Stops - %s:%s" % (HOST_NAME, PORT_NUMBER)
