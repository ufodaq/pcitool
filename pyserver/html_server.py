import json

from optparse import OptionParser, OptionGroup
from multiprocessing import Process

import requests
from api_server import ApiServer

#import flask elements
from flask import render_template
from flask import Flask
from flask import request
from flask import url_for
from flask import redirect
from flask import send_from_directory
from flask import make_response

app = Flask(__name__)
api_server_port = 9000
api_server_host = '0.0.0.0'

@app.route("/json/<command>")
def process_json_command(command):
   headers = {'content-type': 'application/json'}
   message = {'command': command}
   
   for arg in request.args:
      message[arg] = request.args[arg]
      
   r = 0;
   try:
      r = requests.get('http://' + api_server_host + ':' + str(api_server_port),
                       data=json.dumps(message),
                       headers=headers)
   except Exception as e:
      return str(json.dumps({'status':'error', 'description': e}))
   
   #application/octet-stream
   response = make_response(r.content)
   for header in r.headers:
      response.headers[header] = r.headers[header]
      
   return response

#html api
@app.route('/register_info')
def get_register_info():
   #get parameters
   name = request.args.get('name')
   bank = request.args.get('bank')
   
   #load register info
   reg_info = 0
   value = dict()
   try:
      r = requests.get(url_for('process_json_command', 
                               command = 'get_register_info',
                               bank = bank,
                               reg = name, _external = True))
      if(r.json().get('status') == 'error'):
         return 'Error: ' + r.json()['description']
         
      reg_info = r.json()['register']
      
      #get register value
      r = requests.get(url_for('process_json_command', 
                               command = 'read_register',
                               bank = bank,
                               reg = name, _external = True))
      if(r.json().get('status') == 'error'):
         return 'Error: ' + r.json()['description']
         
      value[name] = r.json()['value']
   except Exception as e:
      return str(e)
   
   return render_template('register_info.html',
                          register=reg_info,
                          value=value)
                             
@app.route("/registers_list")
def get_registers_list():
   #get parameters
   bank = request.args.get('bank')
   if not bank is None:
      bank = str(bank)
   
   #load registers list
   reg_list = []
   try:
      r = requests.get(url_for('process_json_command', 
                               command = 'get_registers_list',
                               bank = bank, _external = True))
      if(r.json().get('status') == 'error'):
         return 'Error: ' + r.json()['description']
      reg_list = r.json()['registers']
   except Exception as e:
      return str(e)
   
   #get register values
   value = dict()
   for reg in reg_list:
      try:
         r = requests.get(url_for('process_json_command', 
                                  command = 'read_register',
                                  bank = str(reg['bank']), 
                                  reg = str(reg['name']), _external = True))
         if(r.json().get('status') == 'error'):
            value[reg['name']] = 'Error: ' + r.json()['description']
         else:
            value[reg['name']] = r.json()['value']
         
      except Exception as e:
         value[reg['name']] = 'Error: ' + str(e)

   #render result
   return render_template('registers_list.html',
                          registers = reg_list,
                          render_template = render_template,
                          value = value
                         )

@app.route("/property_info")
def get_property_list():
   #get parameters
   branch = request.args.get('branch')
   if not branch is None:
      branch = str(branch)
   
   #get properties info
   prop_info = 0  
   try:
      r = requests.get(url_for('process_json_command', 
                               command = 'get_property_list',
                               branch = branch, _external = True))
                               
      if(r.json().get('status') == 'error'):
         return 'Error: ' + r.json()['description']
         
      prop_info = r.json()['properties']
      
   except Exception as e:
      return str(e)
   
   value = dict()
   for prop in prop_info:
      try:
         path = prop['path']
         r = requests.get(url_for('process_json_command', 
                                  command = 'get_property',
                                  prop = path, _external = True))
         if(r.json().get('status') == 'error'):
            value[path] = 'Error: ' + r.json()['description']
         else:   
            value[path] = r.json()['value']
            
      except Exception as e:
         value[path] = str(e)

   return render_template('property_info.html',
                          value = value,
                          branch = branch,
                          properties = prop_info
                         )
                         
@app.route("/scripts_info")
def get_scripts_list():
   #get properties info
   prop_info = 0  
   try:
      r = requests.get(url_for('process_json_command', 
                               command = 'get_scripts_list', 
                               _external = True))
                               
      if(r.json().get('status') == 'error'):
         return 'Error: ' + r.json()['description']
         
      scripts_info = r.json()['scripts']
      
   except Exception as e:
      return str(e)

   return render_template('scripts_info.html',
                          scripts = scripts_info
                         )
                         
@app.route("/")
def greet():
   return render_template('base.html',
                          device = device,
                          model = model)

if __name__ == "__main__":
   #parse command line options
   parser = OptionParser()
   parser.add_option("-p", "--port",  action="store",
                     type="int", dest="port", default=5000,
                     help="Set server port (5000)")
                     
   pcilib_group = OptionGroup(parser, "Api server",
                              "Api server options group")
   pcilib_group.add_option("-e", "--external",  action="store_true",
                           dest="external_api_server", 
                           default=False,
                           help="Dont start own api server. Use external"
                                " server instead");                     
   pcilib_group.add_option("--api-server-host", action="store",
                           type="string", dest="api_server_host",
                           default='0.0.0.0',
                           help="Api server ip adress (0.0.0.0)")
   pcilib_group.add_option("--api-server-port", action="store",
                           type="int", dest="api_server_port",
                           default=9000,
                           help="Api server port (9000)")
   pcilib_group.add_option("-d", "--device",  action="store",
                           type="string", dest="device", 
                           default=str('/dev/fpga0'),
                           help="FPGA device (/dev/fpga0)")
   pcilib_group.add_option("-m", "--model",  action="store",
                           type="string", dest="model", default=None,
                           help="Memory model (autodetected)")
                       
   parser.add_option_group(pcilib_group)
                     
   opts = parser.parse_args()[0]
   
   HOST_NAME = '0.0.0.0'
   PORT_NUMBER = opts.port
   
   device = opts.device
   model = opts.model
   
   #start api server in separate process
   api_server_host = opts.api_server_host
   api_server_port = opts.api_server_port
   if(not opts.external_api_server):
      api_server = ApiServer(device, model, (api_server_host, api_server_port))
      def serve_forever(server):
         try:
            server.serve_forever()
         except KeyboardInterrupt:
            pass
      
      Process(target=serve_forever, args=(api_server,)).start()
   
   #start Flask html server
   app.run(host = HOST_NAME, 
           port = PORT_NUMBER,
           threaded=True,
           #debug=True
         )
