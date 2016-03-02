from pcipywrap import *
import os
import sys

class Pcilib(Pcipywrap):
   def __init__(s, *args):
      Pcipywrap.__init__(s, *args)
      
      #load scripts
      scripts_dir = os.environ.get('PCILIB_SCRIPTS_DIR')
      if scripts_dir:
         scripts_dir_abs = os.path.abspath(scripts_dir)
         if not scripts_dir_abs in sys.path:
            sys.path.append(scripts_dir_abs)
         
         s.__scipts = dict()
         for script in os.listdir(scripts_dir_abs):
            if script.endswith('.py'):
               script_module = os.path.splitext(script)[0]
               __import__(script_module)
               s.__scipts[script_module] = sys.modules[script_module]
               
               
   def get_scripts_list(s):
      scripts = []
      for script in s.__scipts:
         curr_script = dict()
         curr_script['name'] = script
         if 'description' in dir(s.__scipts[script]):
            curr_script['description'] = s.__scipts[script].description
         scripts.append(curr_script)
      return scripts
      
      
   def run_script(s, name, input_value):
      if not name in s.__scipts:
         raise Exception('Script ' + name +' has not loaded')
      return s.__scipts[name].run(s, input_value)
