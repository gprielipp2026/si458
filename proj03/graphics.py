import os
import ctypes

#try:
# ctypes.CDLL('/usr/lib/x86_64-linux-gnu/libGL.so.1', mode=ctypes.RTLD_GLOBAL)
#except Exception as e:
#    print(e)

#os.environ['LIBGL_ALWAYS_SOFTWARE'] = '1'
#os.environ['GALLIUM_DRIVER'] = 'llvmpipe' # Extra safety for CPU-only systems
# Disable any specific session type to let WSLg handle it
#if 'XDG_SESSION_TYPE' in os.environ:
#    del os.environ['XDG_SESSION_TYPE']

from OpenGL.GLUT import *
from OpenGL.GL import *
import glfw
import time
from enum import Enum

class Modes(Enum):
    DEFAULT = 1
    PANNING = 2
    SELECTS = 3

class WindowApp:
    def __init__(self, width, height, updateFunc, name='Default Title', fps=24.0, scalingFactor=1.01):
        self.width = width
        self.height = height
        self.updateFunc = updateFunc
        self.fps = float(fps)
        self.scalingFactor = float(scalingFactor)

        self.cursor = (-1, -1)
        self.mode = Modes.DEFAULT
        self.spos = (-1, -1)
        self.epos = (-1, -1)
        self.botLeft = (0, 0)
        self.topRight = (width, height)

        if not glfw.init():
            raise Exception(f'Could not start application "{name}"')

        glfw.window_hint(glfw.CONTEXT_VERSION_MAJOR, 2)
        glfw.window_hint(glfw.CONTEXT_VERSION_MINOR, 1)

        self.window = glfw.create_window(self.width, self.height, name, None, None)

        if not self.window:
            glfw.terminate()
            raise Exception(f'Could not start application "{name}"')
        
        glfw.set_mouse_button_callback(self.window, self.handle_mouse)
        glfw.set_cursor_pos_callback(self.window, self.handle_cursor)
        glfw.set_scroll_callback(self.window, self.handle_scroll)

    def handle_scroll(self, window, x_off, dir):
        print(f'Scroll event:\t{[x_off, dir]}')
        # dir == 1 => zoom in
        # dir == -1 => zoom out
        self.zoom(dir)

    def handle_mouse(self, window, button, action, mods):
        if action == glfw.PRESS:
            self.spos = self.epos = self.cursor
            if button == glfw.MOUSE_BUTTON_LEFT:
                self.mode = Modes.SELECTS
            elif button == glfw.MOUSE_BUTTON_RIGHT:
                self.mode = Modes.PANNING
            print(f'Mouse event: <pressed>\t{self.cursor}')

        elif action == glfw.RELEASE:
            if self.mode == Modes.PANNING:
                self.pan()
            elif self.mode == Modes.SELECTS:
                self.select()
            
            self.mode = Modes.DEFAULT
            print(f'Mouse event: <released>\t{self.cursor}')


    def handle_cursor(self, window, x, y):
        # print(f'Cursor event:\t{[x, y]}')
        # because opengl uses (0,0) at bottom left
        self.cursor = (x, self.height - y)

        if self.mode == Modes.PANNING or self.mode == Modes.SELECTS:
            self.epos = self.cursor
    
    # update the window by moving the top left and bottom right
    def pan(self):
        vect = tuple((e-s for s,e in zip(self.spos, self.epos)))
        self.botLeft = tuple([el + v for el, v in zip(self.botLeft, vect)])
        self.topRight = tuple([el + v for el, v in zip(self.topRight, vect)])
        
        print(f'Pan event:\t{vect}\t{self.botLeft}\t{self.topRight}')

    # update the window by moving the top left and bottom right
    def select(self):
        x1, y1 = self.spos
        x2, y2 = self.epos

        minx, miny = self.botLeft
        maxx, maxy = self.topRight
        width = maxx - minx
        height = maxy - miny

        px = min(x1,x2) / width
        py = min(y1,y2) / height
        self.botLeft = (minx + width * px, miny + height * py)

        px = 1 - (max(x1,x2) / width)
        py = 1 - (max(y1,y2) / height)
        self.topRight = (maxx - width * px, maxy - height * py)

        print(f'Select event:\t{self.botLeft}\t{self.topRight}')

    def zoom(self, dir):
        x, y = self.cursor
        minx, miny = self.botLeft
        maxx, maxy = self.topRight

        zoomF = self.scalingFactor if dir > 0 else (1.0 / self.scalingFactor)

        colP = ((maxx - minx) / zoomF) / 2.0
        rowP = ((maxy - miny) / zoomF) / 2.0

        self.botLeft = (x - rowP, y - colP)
        self.topRight = (x + rowP, y + colP)
        
        print(f'Zoom event:\t{self.botLeft}\t{self.topRight}')

    def start(self):
        glfw.make_context_current(self.window)
        
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1)

        texture = glGenTextures(1)
        glBindTexture(GL_TEXTURE_2D, texture)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST)

        glEnable(GL_BLEND)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)

        renderer = glGetString(GL_RENDERER)
        if renderer:
            renderer = renderer.decode('utf-8')
        version = glGetString(GL_VERSION)
        if version:
            version = version.decode('utf-8')

        print(f'Renderer: {renderer}')
        print(f'Version: {version}')

        while not glfw.window_should_close(self.window):
            left = self.botLeft
            right = self.topRight

            if self.mode == Modes.PANNING:
                vect = tuple((e-s for s,e in zip(self.spos, self.epos)))
                vect = (vect[0] * -1, vect[1])
                left = tuple([el + v for el, v in zip(self.botLeft, vect)])
                right = tuple([el + v for el, v in zip(self.topRight, vect)])

            pixels = np.ascontiguousarray(self.updateFunc(left, right), dtype=np.uint8)

            glClear(GL_COLOR_BUFFER_BIT)

            glColor4f(1.0, 1.0, 1.0, 1.0)

            if (err := glGetError()) != GL_NO_ERROR:
                print(f'OpenGL Error: {err}')

            # Upload the updated NumPy array to the GPU texture
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, self.width, self.height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels)

            # Draw a full-screen quad (rectangle) and map the texture onto it
            glEnable(GL_TEXTURE_2D)
            glBegin(GL_QUADS)
            glTexCoord2f(0, 1); glVertex2f(-1, -1) # Bottom Left
            glTexCoord2f(1, 1); glVertex2f(1, -1)  # Bottom Right
            glTexCoord2f(1, 0); glVertex2f(1, 1)   # Top Right
            glTexCoord2f(0, 0); glVertex2f(-1, 1)  # Top Left
            glEnd()

            if self.mode == Modes.SELECTS:
                x1, y1 = self.spos
                x2, y2 = self.cursor

                xmin, ymin = min(x1, x2), min(y1, y2)
                xmax, ymax = max(x1, x2), max(y1, y2)

                xmin, xmax = (xmin / self.width) * 2.0 - 1.0, (xmax / self.width) * 2.0 - 1.0
                ymin, ymax = (ymin / self.height) * 2.0 - 1.0, (ymax / self.height) * 2.0 - 1.0


                glColor4f(1.0, 0.0, 0.0, 0.6)
                glBegin(GL_QUADS)

                glVertex2f(xmin, ymin) # BL
                glVertex2f(xmax, ymin) # BR
                glVertex2f(xmax, ymax) # TR
                glVertex2f(xmin, ymax) # TL

                glEnd()

            # Swap front and back buffers
            glfw.swap_buffers(self.window)

            # Poll for and process events
            glfw.poll_events()

            # frames / sec
            time.sleep(1.0/self.fps)
            
        self.end()

    def end(self):
        glfw.terminate()


def colorSheet(width, height):
    def update(bl, tr):
        xmin, ymin = bl
        xmax, ymax = tr

        # loop over each pixel
        # Create coordinate grids
        x = np.linspace(xmin, xmax, width)
        y = np.linspace(ymin, ymax, height)
        xv, yv = np.meshgrid(x, y)
        
        pixels = np.zeros((height, width, 3), dtype=np.uint8)
        pixels[:, :, 0] = (xv % 255).astype(np.uint8) # Red channel
        pixels[:, :, 1] = (yv % 255).astype(np.uint8) # Green channel

        return pixels

    return update


def mandel(w, h, maxiter=1000):
    def update(bl, tr):
        # xmin, xmax, ymin, ymax, w, h, maxiter
        # Generate the complex plane
        xmin, ymin = bl
        xmax, ymax = tr

        r1 = np.linspace(xmin, xmax, w)
        r2 = np.linspace(ymin, ymax, h)
        c = r1 + r2[:, None] * 1j
        
        # Initialize arrays
        z = np.zeros_like(c)
        mset = np.zeros(c.shape, dtype=np.int32)
        mask = np.full(c.shape, True, dtype=bool)

        for i in range(maxiter):
            # Update only points that haven't escaped yet
            z[mask] = z[mask]**2 + c[mask]
            
            # Check which points escaped in this iteration
            # Using z.real**2 + z.imag**2 > 4 is faster than np.abs(z) > 2
            escaped = (z.real**2 + z.imag**2) > 4.0
            
            # Record escape iteration for escaped points still in mask
            mset[mask & escaped] = i
            
            # Remove escaped points from future calculations
            mask[escaped] = False
            
            if not mask.any():
                break
                
        # Map iterations to a simple brightness value
        brightness = (mset / maxiter * 255).astype(np.uint8)
        # Stack to create (h, w, 3) where R=G=B
        rgb_array = np.stack([brightness] * 3, axis=-1)    
        
        return rgb_array
    return update

if __name__ == '__main__':
    import numpy as np

    w, h = 360 * 2, 360 * 2
    app = WindowApp(w, h, colorSheet(w, h), fps=30)

    app.start()
