import ti_draw as dr

class Colors:
    BLACK = (0, 0, 0)
    WHITE = (255, 255, 255)
    RED = (255, 0, 0)
    GREEN = (0, 255, 0)
    BLUE = (0, 0, 255)
    YELLOW = (255, 255, 0)
    CYAN = (0, 255, 255)
    MAGENTA = (255, 0, 255)
    GRAY = (128, 128, 128)

class Graphics:
    @staticmethod
    def set_color(color):
        """Set the drawing color using a tuple (R, G, B)."""
        dr.set_color(color[0], color[1], color[2])

    @staticmethod
    def draw_rect(x, y, w, h, color=None, fill=False):
        if color:
            Graphics.set_color(color)
        if fill:
            dr.fill_rect(x, y, w, h)
        else:
            dr.draw_rect(x, y, w, h)

    @staticmethod
    def draw_circle(x, y, r, color=None, fill=False):
        if color:
            Graphics.set_color(color)
        if fill:
            dr.fill_circle(x, y, r)
        else:
            dr.draw_circle(x, y, r)
            
    @staticmethod
    def draw_text(x, y, text, color=None):
        if color:
            Graphics.set_color(color)
        dr.draw_text(x, y, str(text))
        
    @staticmethod
    def fill_screen(color):
        Graphics.draw_rect(0, 0, 318, 212, color, fill=True)
