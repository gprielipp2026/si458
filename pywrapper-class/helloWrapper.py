#python wrapper for hello.c

"""
Things to know/need

1. shared object of the C file
2. argument types of the function
3. return types of the function
4. depending on types, casting must occur
"""

from ctypes import *

def hello_wrap():
    # grab shared object file
    # gcc -shared -o hello.so -fPIC hello.c
    lib = CDLL('./hello.so')
    hello = lib.hello # func pointer
    
    # define return type
    hello.restype = c_char_p # char*

    # call the function
    greeting = hello()

    return greeting.decode('UTF-8')



if __name__ == '__main__':
    greeting = hello_wrap()
    print(greeting)
