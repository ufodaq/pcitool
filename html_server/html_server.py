import pcipywrap
import json
from optparse import OptionParser

#import flask elements
from flask import render_template
from flask import Flask
from flask import request
from flask import url_for
from flask import redirect
from flask import send_from_directory

app = Flask(__name__)
pcilib = 0;
device = '/dev/fpga0'
model = ''

# property json api
@app.route("/property_info_json")
def get_property_list_json():
   branch = request.args.get('branch')
   if not branch is None:
      branch = str(branch)
   
   prop_info = 0
   try:
      prop_info = pcilib.get_property_list(branch)
      return json.dumps(prop_info)
   except Exception as e:
      return json.dumps({'error': str(e)})

@app.route('/get_property_json')
def get_property_json():
   prop = request.args.get('prop')

   try:
      val = pcilib.get_property(str(prop))
      return json.dumps({'value': val})
   except Exception as e:
      return json.dumps({'error': str(e)})
      
@app.route('/set_property_json')
def set_property_json():
   val = request.args.get('val')
   prop = request.args.get('prop')

   try:
      pcilib.set_property(float(val), str(prop))
      return json.dumps({'status': 'ok'})
   except Exception as e:
      return json.dumps({'error': str(e)})
      
# register json api
@app.route("/registers_list_json")
def get_registers_list_json():
   reg_list = 0
   try:
      reg_list = pcilib.get_registers_list()
      return json.dumps(reg_list)
   except Exception as e:
      return json.dumps({'error': str(e)})

@app.route('/read_register_json')
def read_register_json():
   name = request.args.get('name')
   bank = request.args.get('bank')
   
   try:
      value = pcilib.read_register(str(name), str(bank))
      return json.dumps({'value': value})
   except Exception as e:
      return json.dumps({'error': str(e)})

@app.route('/write_register_json')
def write_register_json():
   val = request.args.get('val')
   name = request.args.get('name')
   bank = request.args.get('bank')

   try:
      pcilib.write_register(float(val), str(name), str(bank))
      return json.dumps({'status': 'ok'})
   except Exception as e:
      return json.dumps({'error': str(e)})

#html api
@app.route('/set_property')
def set_property():
   val = request.args.get('val')
   prop = request.args.get('prop')

   try:
      pcilib.set_property(float(val), str(prop))
      return redirect(url_for('get_property_list', branch=prop))
   except Exception as e:
      return str(e)
   
@app.route('/write_register')
def write_register():
   val = request.args.get('val')
   name = request.args.get('name')
   bank = request.args.get('bank')

   try:
      pcilib.write_register(float(val), str(name), str(bank))
      return redirect(url_for('get_register_info', name=name, bank=bank))
   except Exception as e:
      return str(e)

@app.route('/register_info')
def get_register_info():
   name = request.args.get('name')
   bank = request.args.get('bank')
   
   reg_info = 0
   value = dict()
   try:
      reg_info = pcilib.get_register_info(str(name), str(bank))
      value[name] = pcilib.read_register(str(name), str(bank))
   except Exception as e:
      return str(e)
   return render_template('register_info.html',
                          register=reg_info,
                          value=value)
                             
@app.route("/registers_list")
def get_registers_list():
   bank = request.args.get('bank')
   if not bank is None:
      bank = str(bank)
      
   reg_list = 0
   try:
      reg_list = pcilib.get_registers_list(bank)
   except Exception as e:
      return str(e)
   
   value = dict()
   for reg in reg_list:
      print reg
      try:
         value[reg['name']] = pcilib.read_register(str(reg['name']),
                                                   str(reg['bank']))
      except Exception as e:
         value[reg['name']] = str(e)

   return render_template('registers_list.html',
                          registers = reg_list,
                          render_template = render_template,
                          value = value
                         )

@app.route("/property_info")
def get_property_list():
   branch = request.args.get('branch')
   if not branch is None:
      branch = str(branch)
   
   prop_info = 0
   try:
      prop_info = pcilib.get_property_list(branch)
   except Exception as e:
      return str(e)
   
   value = dict()
   if (len(prop_info) == 1) and not ('childs' in (prop_info[0])['flags']):
      try:
         branch = (prop_info[0])['path']
         value[branch] = pcilib.get_property(branch)
      except Exception as e:
         return str(e) 
   else:
      for prop in prop_info:
         try:
            path = prop['path']
            value[path] = pcilib.get_property(path)
         except Exception as e:
            value[path] = str(e)

   return render_template('property_info.html',
                          value = value,
                          branch = branch,
                          properties = prop_info,
                          json = json
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
   parser.add_option("-d", "--device",  action="store",
                     type="string", dest="device", default=str('/dev/fpga0'),
                     help="FPGA device (/dev/fpga0)")                     
   parser.add_option("-m", "--model",  action="store",
                     type="string", dest="model", default=None,
                     help="Memory model (autodetected)")
   opts = parser.parse_args()[0]
   
   HOST_NAME = '0.0.0.0'
   PORT_NUMBER = opts.port
   
   device = opts.device
   model = opts.model
   
   pcilib = pcipywrap.Pcipywrap(device, model)
   pcipywrap.__redirect_logs_to_exeption()
   app.run(host = HOST_NAME, port = PORT_NUMBER)
