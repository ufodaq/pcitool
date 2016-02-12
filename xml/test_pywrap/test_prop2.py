import pcipywrap
    
def read_from_register():    
    return pcipywrap.get_property('/registers/fpga/reg1') / 2
    
def write_to_register(value):    
    pcipywrap.set_property(value*3, '/registers/fpga/reg1')

