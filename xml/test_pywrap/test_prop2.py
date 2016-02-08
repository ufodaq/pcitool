import pcipywrap
    
def read_from_register():    
    return pcipywrap.get_property('/registers/fpga/reg1') / 3
    
def write_to_register(value):    
    pcipywrap.set_property('/registers/fpga/reg1', value*3)

