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
    def __init__(self, width, height, updateFunc, name='Default Title', fps=24.0, scalingFactor=5.0):
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
        print(f'Mouse event:\t{[button, action, mods]}')
        if action == glfw.PRESS:
            self.spos = self.cursor
            if button == glfw.MOUSE_BUTTON_LEFT:
                self.mode = Modes.SELECTS
            elif button == glfw.MOUSE_BUTTON_RIGHT:
                self.mode = Modes.PANNING

        elif action == glfw.RELEASE:
            if self.mode == Modes.PANNING:
                self.pan()
            elif self.mode == Modes.SELECTS:
                self.select()
            
            self.mode = Modes.DEFAULT

    def handle_cursor(self, window, x, y):
        # print(f'Cursor event:\t{[x, y]}')
        self.cursor = (x, y)

        if self.mode == Modes.PANNING or self.mode == Modes.SELECTS:
            self.epos = self.cursor
    
    # update the window by moving the top left and bottom right
    def pan(self):
        vect = (e-s for s,e in zip(self.spos, self.epos))
        self.botLeft = tuple([el + v for el, v in zip(self.botLeft, vect)])
        self.topRight = tuple([el + v for el, v in zip(self.topRight, vect)])
        
        print(f'Pan event:\t{self.botLeft}\t{self.topRight}')

    # update the window by moving the top left and bottom right
    def select(self):
        if any([b < a for a,b in zip(self.spos, self.epos)]):
            self.spos, self.epos = self.epos, self.spos
        
        self.botLeft = self.spos
        self.topRight = self.epos

        print(f'Select event:\t{self.botLeft}\t{self.topRight}')

    def zoom(self, dir):
        # translate cursor to center screen
        trans = (e-s for s, e in zip(self.cursor, (self.width/2.0, self.height/2.0)))
        self.botLeft = tuple([el + v for el, v in zip(self.botLeft, trans)])
        self.topRight = tuple([el + v for el, v in zip(self.topRight, trans)])

        # scale based on dir
        scale = (self.scalingFactor * dir, self.scalingFactor * dir)
        self.botLeft = tuple([el + v for el, v in zip(self.botLeft, scale)])
        self.topRight = tuple([el - v for el, v in zip(self.topRight, scale)])
        
        print(f'Zoom event:\t{self.botLeft}\t{self.topRight}')

    def start(self):
        glfw.make_context_current(self.window)
        
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1)

        texture = glGenTextures(1)
        glBindTexture(GL_TEXTURE_2D, texture)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST)

        while not glfw.window_should_close(self.window):
            pixels = self.updateFunc(self.botLeft, self.topRight)

            glClear(GL_COLOR_BUFFER_BIT)

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

            if self.mode == Modes.PANNING:
                xmin, ymin = self.spos
                xmax, ymax = self.epos

                xmin, xmax = xmin / self.width, xmax / self.width
                ymin, ymax = ymin / self.height, ymax / self.height

                glBegin(GL_LINES)
                glVertex2f(xmin, ymin)
                glVertex2f(xmax, ymax)
                glEnd()

            elif self.mode == Modes.SELECTS:
                xmin, ymin = self.botLeft
                xmax, ymax = self.topRight

                xmin, xmax = xmin / self.width, xmax / self.width
                ymin, ymax = ymin / self.height, ymax / self.height

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
        pixels = np.zeros((width, height, 3))

        xmin, ymin = bl
        xmax, ymax = tr

        # loop over each pixel
        for y in range(height):
            for x in range(width):
                percX = x / float(width)
                percY = y / float(height)

                color = (int((xmax - xmin)*percX + xmin) % 255, int((ymax - ymin)*percY + ymin) % 255, 0)

                pixels[y][x] = color

        return pixels

    return update


if __name__ == '__main__':
    import numpy as np

    w, h = 360 * 2, 360 * 2
    app = WindowApp(w, h, colorSheet(w, h), fps=30)

    app.start()