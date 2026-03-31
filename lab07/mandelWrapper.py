#!/usr/bin/env python3

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
 
    array = array.reshape((h.value, w.value))

    return array
    



if __name__ == "__main__":
    # grab array from mandelSet
    w = 1000
    h = 850
    array = mandelSet(w,h, p=2)

    # plot code goes below
    plt.imshow(array) 
    plt.title('Mandelbrot Set')
    plt.show()

