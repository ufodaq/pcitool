import sys
if sys.version_info >= (3,0):
   import binascii
   
def run(ctx, input):
   if sys.version_info >= (3,0):
      return binascii.a2b_uu('111')
   else:
      return bytearray(str(input) +'11\0\1\n')
   
