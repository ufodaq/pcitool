import pcipywrap
import os
    
def read_from_register(): 
    reg1_val =  pcipywrap.read_register('reg1');     
    test_prop1_val = pcipywrap.read_register('test_prop3');
    return test_prop1_val - reg1_val;
