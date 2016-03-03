def read_from_register(ctx, value):
   return ctx.get_property('/test/prop3') / 2

def write_to_register(ctx, value):
    ctx.set_property(value*2, '/test/prop3')
