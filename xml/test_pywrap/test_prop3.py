import pcipywrap

def read_from_register(value):
   return pcipywrap.get_property('/registers/fpga/reg1')
    
def write_to_register(value):
    pcipywrap.set_property(value, '/registers/fpga/reg1')
