import pcipywrap

def read_from_register(value):
   return pcipywrap.get_property('/test/prop3') / 2
    
def write_to_register(value):
    pcipywrap.set_property(value*2, '/test/prop3')

