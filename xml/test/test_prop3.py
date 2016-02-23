def read_from_register(ctx, value):
   return ctx.get_property('/registers/fpga/sensor_temperature') + 500

def write_to_register(ctx, value):
   ctx.set_property(value, '/registers/fpga/sensor_temperature') - 500
