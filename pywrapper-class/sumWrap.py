#python wrapper for hello.c

"""
Things to know/need

1. shared object of the C file
2. argument types of the function
3. return types of the function
4. depending on types, casting must occur
"""

from ctypes import *

def sum_wrap(x, y):
    # grab shared object file
    # gcc -shared -o hello.so -fPIC hello.c
    lib = CDLL('./sum.so')
    sumc = lib.sum # func pointer
   
    sumc.args = [c_uint8, c_uint8]

    # define return type
    sumc.restype = c_uint16 # char*

    # call the function

    return sumc(x, y)



if __name__ == '__main__':
    print(sum_wrap(10, 12))
