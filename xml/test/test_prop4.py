import time
import threading
lock = threading.Lock()

def read_from_register(ctx, value):
   with lock:
      ctx.lock('test_prop4')

      cur = read_from_register.counter
      read_from_register.counter += 1
      for i in range (0, 5):
         time.sleep(0.1)
      out = ctx.get_property('/test/prop3') / 2

      ctx.unlock('test_prop4')

      return out

read_from_register.counter = 0

def write_to_register(ctx, value):
   with lock:
      ctx.set_property(value*2, '/test/prop3')
