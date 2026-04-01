#!/usr/bin/env python3
import sys
import os
from ctypes import *
import numpy as np
import matplotlib
import matplotlib.pyplot as plt
import matplotlib.patches as patches
import subprocess

matplotlib.use('TkAgg')

def check_lib(file):
    if not os.path.isfile(file):
        subprocess.call('make')

def mandelSet(w, h, xmax=1.0, xmin=-2.0, ymax=1.0, ymin=-1.0, p=1):
    check_lib('./mandel.so')

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
    check_lib('./mandel.so')

    lib = CDLL('./mandel.so')

    func = lib.print
    func.args = [POINTER(c_uint8), c_uint32, c_uint32]
    func.restype = c_void_p

    ptr = arr.ctypes.data_as(POINTER(c_uint8))
    w = c_uint32(w)
    h = c_uint32(h)

    func(ptr, w, h)

"""
Have xmin, xmax, ymin, ymax defined by selected area
"""

def linterp(x, start, end, new_start, new_end):
    # calculate percentage between start and end
    percent = x / (end - start)

    # apply percentage to new start and end
    return new_start + (percent * (new_end - new_start))

class InteractiveMandelbrot:
    def __init__(self, w, h, p):
        self.reset()

        self.w = w
        self.h = h
        self.p = p

        self.fig, self.ax = plt.subplots()
        self.img = self.rect = None
        
        self.register()

    def reset(self):
        self.xmin, self.xmax, self.ymin, self.ymax = [-2.0, 1.0, -1.0, 1.0]
        self.drag_start = self.drag_end = None
        self.rect = None

    def register(self):
        self.fig.canvas.mpl_connect('button_press_event', self.on_press())
        self.fig.canvas.mpl_connect('motion_notify_event', self.on_move())
        self.fig.canvas.mpl_connect('button_release_event', self.on_release())
        self.fig.canvas.mpl_connect('close_event', self.save())

    def update(self):
        array = mandelSet(self.w, self.h, xmax=self.xmax, xmin=self.xmin, ymax=self.ymax, ymin=self.ymin, p=self.p)

        if self.img:
            self.img.set_data(array)
        else:
            self.img = self.ax.imshow(array, cmap='gray')

        if self.drag_start and self.drag_end:
            # overlay the selected area with a faint red rectangle
            x, y = self.drag_start
            w, h = [e-s for s,e in zip(self.drag_start, self.drag_end)]
            self.rect = patches.Rectangle((x, y), w, h, facecolor='red', alpha=0.4)
            self.ax.add_patch(self.rect)
        elif self.rect is not None:
            # remove the rect when done
            self.rect.remove()
            self.rect = None

    def screen_to_world(self, start, end):
        """
        start < end
        x = index 0
        y = index 1
        """ 
        self.xmin = linterp(start[0], 0, self.w, self.xmin, self.xmax) 
        self.xmax = linterp(  end[0], 0, self.w, self.xmin, self.xmax) 
        self.ymin = linterp(start[1], 0, self.h, self.ymin, self.ymax) 
        self.ymax = linterp(  end[1], 0, self.h, self.ymin, self.ymax) 
    
    def on_press(self):
        def handler(event):
            if event.inaxes:
                self.drag_start = self.drag_end = (event.xdata, event.ydata)
                self.update()
        return handler

    def on_move(self):
        def handler(event):
            if event.inaxes:
                self.drag_end = (event.xdata, event.ydata)

                if self.drag_start is not None and self.drag_end is not None:
                    if any([e<s for s,e in zip(self.drag_start, self.drag_end)]):
                        self.drag_start, self.drag_end = self.drag_end, self.drag_start

                self.update()
        return handler

    def on_release(self):
        def handler(event):
            if event.inaxes:
                self.drag_end = (event.xdata, event.ydata)

                if self.drag_start is not None and self.drag_end is not None:
                    if any([s<e for s,e in zip(self.drag_start, self.drag_end)]):
                        self.drag_start, self.drag_end = self.drag_end, self.drag_start

                    self.screen_to_world(self.drag_start, self.drag_end)

                self.drag_start = self.drag_end = None
                self.update()
        return handler

    def start(self):
        self.update()
        plt.title(f'{self.w}x{self.h} Mandelbrot')
        plt.show()

    def save(self):
        def handler(event):
            os.makedirs('imgs', exist_ok=True)

            filename = f'imgs/{self.w}x{self.h}_({self.xmin} - {self.xmax})_({self.ymin} - {self.ymax}).png'
            event.canvas.figure.savefig(filename, bbox_inches='tight', dpi=300)
        return handler

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print(f'usage: {sys.argv[0]} <width> <height> <processors>')
        sys.exit(0)

    # grab array from mandelSet
    w = int(sys.argv[1]) 
    h = int(sys.argv[2])
    p = int(sys.argv[3])

    #array = mandelSet(w,h, p=p)

    #cprintMandel(array, w, h)

    # plot code goes below
    #filename = f'imgs/{w}x{h}.png' 
    #plt.imsave(filename, array, cmap='gray', dpi=300)

    #plt.imshow(array, cmap='gray')
    #plt.show()

    app = InteractiveMandelbrot(w, h, p)
    app.start()
