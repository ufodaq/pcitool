import time
import os #delete later
import pcipywrap
import json
import BaseHTTPServer

class PcilibServerHandler(BaseHTTPServer.BaseHTTPRequestHandler):
	
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
                     
         elif(command == 'open'):
            #check required arguments
            if not 'device' in data:
               s.error('message doesnt contains "device" field, '
                       'which is required for "open" command', data)
               return
            #parse command arguments and convert them to string
            device = str(data.get('device', None))
            model = data.get('model', None)
            if not model is None:
				model = str(model)
            
            try:
               s.openPcilibInstance(device, model)
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
                   
         elif(command == 'get_registers_list'):
            #parse command arguments and convert them to string
            bank = data.get('bank', None)
            if not bank is None:
               bank = str(bank)

            registers = dict()
            try:
               registers = pcipywrap.get_registers_list(bank)
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
               register = pcipywrap.get_register_info(reg, bank)
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
		 
         elif(command == 'get_property_info'):   
            #parse command arguments and convert them to string
            branch = data.get('branch', None)
            if not branch is None:
				branch = str(bank)
            
            properties = dict()
            try:
               properties = pcipywrap.get_property_info(branch)
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
      pcipywrap.closeCurrentPcilibInstance()
      
      lib = pcipywrap.createPcilibInstance(device, model)
      pcipywrap.setPcilib(lib)
         
   """Send help message"""
   def help(s, received_message = None):
      s.send_response(200)
      s.send_header('content-type', 'application/json')
      s.end_headers()
      out = {'status': 'ok', 'help' : 'under construction'}
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

HOST_NAME = '' # !!!REMEMBER TO CHANGE THIS!!!
PORT_NUMBER = 12412 # Maybe set this to 9000.

if __name__ == '__main__':
   #initialize variables test (to remove)
   os.environ["APP_PATH"] = '/home/vchernov/1215N/pcitool'
   os.environ["PCILIB_MODEL_DIR"] = os.environ["APP_PATH"] + "/xml"
   os.environ["LD_LIBRARY_PATH"] = os.environ["APP_PATH"] + "/pcilib"
   
   pcilib_server = BaseHTTPServer.HTTPServer
   httpd = pcilib_server((HOST_NAME, PORT_NUMBER), PcilibServerHandler)
   print time.asctime(), "Server Starts - %s:%s" % (HOST_NAME, PORT_NUMBER)
   try:
      httpd.serve_forever()
   except KeyboardInterrupt:
      pass
   httpd.server_close()
   print time.asctime(), "Server Stops - %s:%s" % (HOST_NAME, PORT_NUMBER)
