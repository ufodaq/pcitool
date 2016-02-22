import pcipywrap
import json

#import flask elements
from flask import render_template
from flask import Flask
from flask import request
from flask import url_for
from flask import redirect

app = Flask(__name__)
pcilib = 0;
device = '/dev/fpga0'
model = 'test_pywrap'

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
   value = 0
   try:
      reg_info = pcilib.get_register_info(str(name), str(bank))
      value = pcilib.read_register(str(name), str(bank))
   except Exception as e:
      return str(e)
   return render_template('register_info.html',
                          register=reg_info,
                          value=value)
                             

@app.route("/registers_list")
def get_registers_list():
   reg_list = 0
   try:
      reg_list = pcilib.get_registers_list()
   except Exception as e:
      return str(e)

   return render_template('registers_list.html',
                          registers=reg_list,
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
   
   value = -1
   if (len(prop_info) == 1) and not ('childs' in (prop_info[0])['flags']):
      try:
         branch = (prop_info[0])['path']
         value = pcilib.get_property(branch)
      except Exception as e:
         return str(e)

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
   pcilib = pcipywrap.Pcipywrap(device, model)
   pcipywrap.__redirect_logs_to_exeption()
   app.debug = True
   app.run()
