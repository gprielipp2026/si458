#!/usr/bin/env python3
import sys
from ctypes import *
import numpy as np
import matplotlib.pyplot as plt

def mandelSet(w, h, xmax=1.0, xmin=-2.0, ymax=1.0, ymin=-1.0, p=1):
    if p < 1:
        p = 1
    
    xmax, xmin, ymax, ymin = list(map(lambda x: c_double(x), [xmax, xmin, ymax, ymin]))
    w, h, p = list(map(lambda x: c_uint32(x), [w, h, p]))

    lib = CDLL('./mandel.so')

    func = lib.mandelSet

    func.args = [POINTER(c_uint8), c_uint32, c_uint32, c_double, c_double, c_double, c_double, c_uint32]
    func.restype = c_void_p

    array = np.zeros(h.value * w.value, dtype=np.uint8)
    ptr = array.ctypes.data_as(POINTER(c_uint8))

    func(ptr, w, h, xmax, xmin, ymax, ymin, p)
 
    array = array.reshape((w.value, h.value))

    return array
    
def cprintMandel(arr, w, h):
    lib = CDLL('./mandel.so')

    func = lib.print
    func.args = [POINTER(c_uint8), c_uint32, c_uint32]
    func.restype = c_void_p

    ptr = arr.ctypes.data_as(POINTER(c_uint8))
    w = c_uint32(w)
    h = c_uint32(h)

    func(ptr, w, h)

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print(f'usage: {sys.argv[0]} <width> <height> <processors>')
        sys.exit(0)

    # grab array from mandelSet
    w = int(sys.argv[1]) 
    h = int(sys.argv[2])
    p = int(sys.argv[3])

    array = mandelSet(w,h, p=p)

    #cprintMandel(array, w, h)

    # plot code goes below
    #filename = f'imgs/{w}x{h}.png' 
    #plt.imsave(filename, array, cmap='gray', dpi=300)

    plt.imshow(array, cmap='gray')
    plt.show()

