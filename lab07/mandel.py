import numpy as np
import matplotlib.pyplot as plt

def inMandel(x, y):
    c = x + y*1j
    Zn = 0

    for _ in range(100):
        Zn = Zn*Zn + c
    
    return int(np.abs(Zn) < 2)

def mandelSet(w, h, xmax=1.0, xmin=-2.0, ymax=1.0, ymin=-1.0):
    array = []

    dx = (xmax - xmin) / float(w)
    dy = (ymax - ymin) / float(h)

    for y in range(h):
        row = []
        for x in range(w):
            row.append( inMandel(xmin + x*dx, ymin + y*dy) )
        array.append(row)
    
    return array




if __name__ == "__main__":
    # grab array from mandelSet
    w = 1000
    h = 850
    array = mandelSet(w,h)

    # plot code goes below
    plt.imshow(array) 
    plt.title('Mandelbrot Set')
    plt.show()

