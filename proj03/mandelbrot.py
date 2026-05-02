from graphics import WindowApp
from multiprocessing import Process, Array
import numpy as np

def py_mandel_set(w, h, maxiter=100):
    def update(bl, tr):
        pixels = [[(255, 255, 255) for _ in range(w)] for _ in range(h)]

        minx, miny = bl
        maxx, maxy = tr

        dx = float(maxx - minx) / float(w)
        dy = float(maxy - miny) / float(h)

        for y in range(h):
            for x in range(w):
                c = (x*dx + minx) + (y*dy + miny) * 1j
                z = 0
                for i in range(maxiter):
                    z = z*z + c

                mag = abs(z)
                if mag < 2:
                    pixels[y][x] = (0,0,0)

        return pixels

    return update

def mp_mandel_set(w, h, PEs=32, maxiter=100):

    # actually computes the mandelbrot set
    def worker(minx, miny, pixels, sx, sy, ex, ey, dx, dy):
        for y in range(sy, ey):
            for x in range(sx, ex):
                c = (x*dx + minx) + (y*dy + miny) * 1j
                z = 0
                for i in range(maxiter):
                    z = z*z + c

                mag = 3
                try:
                    mag = abs(z)
                except:
                    pass
                idx = (x + y * h) * 3
                if mag < 2:
                    pixels[idx:idx+3] = [0,0,0]
                else:
                    pixels[idx:idx+3] = [255, 255, 255]

    def update(bl, tr):
        minx, miny = bl
        maxx, maxy = tr
        pixels = Array('i', w * h * 3, lock=False)
        mdx = float(maxx - minx) / float(w)
        mdy = float(maxy - miny) / float(h)


        dy = int(h / PEs)
        sy, ey = 0, 0
        lx, ly = w % PEs, h % PEs
        pool = []
        for PE in range(PEs):
            ey = sy + dy + (1 if ly > 0 else 0)

            p = Process(target=worker, args=(minx, miny, pixels, 0, sy, w, ey, mdx, mdy))
            pool.append( p )
            p.start()

            lx -= 1
            ly -= 1
            sy = ey
        
        for PE in pool:
            PE.join()

        return pixels


    return update
    


if __name__ == '__main__':
    w, h = 800, 800
    app = WindowApp(w, h, mp_mandel_set(w,h), fps=12, name='Mandelbrot Set', botLeft=(-2, -1), topRight=(1, 1))
    app.start()
