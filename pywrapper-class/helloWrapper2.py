#python wrapper for hello.c

"""
Things to know/need

1. shared object of the C file
2. argument types of the function
3. return types of the function
4. depending on types, casting must occur
"""

from ctypes import *
from random import randint

def hello_wrap2(x):
    # grab shared object file
    # gcc -shared -o hello2.so -fPIC hello2.c
    lib = CDLL('./hello2.so')
    hello = lib.hello # func pointer
   
    # set argument types
    hello. args = [c_int]

    # define return type
    hello.restype = c_char_p # char*

    # call the function
    greeting = hello(x)

    return greeting.decode('UTF-8')



if __name__ == '__main__':
    for i in range(10):
        greeting = hello_wrap2(randint(1, 100))
        print(greeting)
