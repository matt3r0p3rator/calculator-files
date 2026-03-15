# ==========================
# BUNDLED TI-NSPIRE APP
# ==========================

import ti_draw
import ti_system


class App:
    def __init__(self, width=318, height=212):
        self.width = width
        self.height = height
        self.running = False
        self.current_scene = None

    def set_scene(self, scene):
        self.current_scene = scene
        if self.current_scene:
            self.current_scene.app = self
            self.current_scene.setup()

    def run(self):
        
        ti_draw.clear()
        ti_draw.set_window(0, self.width, self.height, 0)
        
        self.running = True
        
        while self.running:
            if self.current_scene:
                self.current_scene.handle_input()
                self.current_scene.update()
                
                ti_draw.clear()
                self.current_scene.draw()
            else:
                self.running = False


class Scene:
    def __init__(self):
        self.app = None

    def setup(self):
        pass

    def handle_input(self):
        pass

    def update(self):
        pass

    def draw(self):
        pass



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
        ti_draw.set_color(color[0], color[1], color[2])

    @staticmethod
    def draw_rect(x, y, w, h, color=None, fill=False):
        if color:
            Graphics.set_color(color)
        if fill:
            ti_draw.fill_rect(x, y, w, h)
        else:
            ti_draw.draw_rect(x, y, w, h)

    @staticmethod
    def draw_circle(x, y, r, color=None, fill=False):
        if color:
            Graphics.set_color(color)
        if fill:
            ti_draw.fill_circle(x, y, r)
        else:
            ti_draw.draw_circle(x, y, r)
            
    @staticmethod
    def draw_text(x, y, text, color=None):
        if color:
            Graphics.set_color(color)
        ti_draw.draw_text(x, y, str(text))
        
    @staticmethod
    def fill_screen(color):
        Graphics.draw_rect(0, 0, 318, 212, color, fill=True)



class Keys:
    UP = "up"
    DOWN = "down"
    LEFT = "left"
    RIGHT = "right"
    ENTER = "enter"
    ESC = "esc"
    TAB = "tab"
    DEL = "del"
    CLEAR = "clear"
    SPACE = "space"

class Input:
    @staticmethod
    def get_key():
        """Returns the current key being pressed."""
        return ti_system.get_key()
    
    @staticmethod
    def is_key_pressed(key):
        """Check if a specific key is pressed."""
        return ti_system.get_key() == key




def solve_gauss_jordan(matrix):
    """
    Pure function to solve an augmented matrix using Gauss-Jordan elimination.
    Returns the solution as a list of floats, or None if no unique solution exists.
    """
    generator = solve_gauss_jordan_steps(matrix)
    result = None
    for step in generator:
        if step['type'] == 'solution':
            result = step['data']
    return result

def solve_gauss_jordan_steps(matrix):
    """
    A generator that yields the steps of Gauss-Jordan elimination.
    Useful for building UIs, calculators, or step-by-step CLIs.
    Yields dictionaries with 'type', 'msg', 'matrix', and other step-specific data.
    """
    mat = [list(row) for row in matrix]
    n_rows = len(mat)
    n_cols = len(mat[0])

    yield {'type': 'start', 'msg': 'Starting augmented matrix', 'matrix': [list(row) for row in mat]}

    for col in range(n_rows):
        yield {'type': 'col_start', 'col': col, 'msg': "Working on column {}".format(col + 1), 'matrix': [list(row) for row in mat]}

        # Swap
        if mat[col][col] == 0:
            swapped = False
            for swap_row in range(col + 1, n_rows):
                if mat[swap_row][col] != 0:
                    mat[col], mat[swap_row] = mat[swap_row], mat[col]
                    yield {
                        'type': 'swap', 
                        'row1': col, 
                        'row2': swap_row, 
                        'msg': "R{} <-> R{}".format(col+1, swap_row+1), 
                        'matrix': [list(row) for row in mat]
                    }
                    swapped = True
                    break
            if not swapped:
                yield {'type': 'error', 'msg': 'No valid pivot found. The system may have no unique solution.', 'matrix': [list(row) for row in mat]}
                yield {'type': 'solution', 'data': None}
                return

        # Scale
        pivot_val = mat[col][col]
        if pivot_val != 1.0:
            mat[col] = [x / pivot_val for x in mat[col]]
            yield {
                'type': 'scale', 
                'row': col, 
                'factor': pivot_val, 
                'msg': "R{}  ->  (1 / {:.4f}) * R{}".format(col+1, pivot_val, col+1), 
                'matrix': [list(row) for row in mat]
            }

        # Eliminate
        for row in range(n_rows):
            if row == col:
                continue
            factor = mat[row][col]
            if factor != 0:
                mat[row] = [mat[row][i] - factor * mat[col][i] for i in range(n_cols)]
                sign = "-" if factor >= 0 else "+"
                yield {
                    'type': 'eliminate', 
                    'row': row, 
                    'pivot_row': col, 
                    'factor': factor, 
                    'msg': "R{}  ->  R{}  {}  ({:.4f} * R{})".format(row+1, row+1, sign, abs(factor), col+1), 
                    'matrix': [list(row) for row in mat]
                }

    # Check solution
    solution = []
    for row in range(n_rows):
        left_all_zero = all(abs(mat[row][i]) < 1e-9 for i in range(n_rows))
        if left_all_zero:
            if abs(mat[row][-1]) > 1e-9:
                yield {'type': 'error', 'msg': 'Inconsistent system - No solution exists.', 'matrix': [list(row) for row in mat]}
                yield {'type': 'solution', 'data': None}
                return
            else:
                yield {'type': 'error', 'msg': 'Infinite solutions exist (dependent system).', 'matrix': [list(row) for row in mat]}
                yield {'type': 'solution', 'data': None}
                return
        solution.append(mat[row][-1])

    yield {'type': 'done', 'msg': 'Reduced Row Echelon Form (RREF) reached', 'matrix': [list(row) for row in mat]}
    yield {'type': 'solution', 'data': solution}

def verify_solution(original_matrix, solution):
    """
    Verifies if a given solution satisfies the original augmented matrix.
    Returns (all_correct, results_list)
    """
    n_vars = len(solution)
    all_correct = True
    results = []

    for i, row in enumerate(original_matrix):
        lhs = sum(row[j] * solution[j] for j in range(n_vars))
        rhs = row[-1]
        ok = abs(lhs - rhs) < 1e-6
        if not ok:
            all_correct = False
        results.append({
            'equation_idx': i,
            'lhs': lhs,
            'rhs': rhs,
            'ok': ok,
            'row': row
        })
        
    return all_correct, results



class MatrixViewer(Scene):
    def setup(self):
        self.matrix = [
            [2, 1, -1, 8],
            [-3, -1, 2, -11],
            [-2, 1, 2, -3]
        ]
        self.all_steps = list(solve_gauss_jordan_steps(self.matrix))
        self.current_step = 0
        

    def handle_input(self):
        key = Input.get_key()
        if key == Keys.RIGHT:
            if self.current_step < len(self.all_steps) - 1:
                self.current_step += 1
        elif key == Keys.LEFT:
            if self.current_step > 0:
                self.current_step -= 1
        elif key == Keys.ESC:
            self.app.running = False

    def display_matrix(self, state):
        mat = state.get('matrix', [])
        if not mat:
            return
        
        y = 50
        for row in mat:
            s_parts = []
            for i, x in enumerate(row):
                if i == len(row) - 1:
                    s_parts.append("| {:6.2f}".format(float(x)))
                    s_parts.append("{:6.2f}".format(float(x)))
            s = "  ".join(s_parts)
            Graphics.draw_text(10, y, "[ {} ]".format(s), Colors.WHITE)
            y += 20
            
    def draw(self):
        Graphics.fill_screen(Colors.BLACK)
        
        state = self.all_steps[self.current_step]
        msg = state.get('msg', 'Processing...')
        
        Graphics.draw_text(10, 10, "Gauss-Jordan Elimination (NumPy)", Colors.CYAN)
        Graphics.draw_text(10, 30, msg, Colors.YELLOW)
        Graphics.draw_text(10, 190, "LEFT/RIGHT: Prev/Next Step | ESC: Exit", Colors.GREEN)
        
        if state.get('type') == 'solution' and state.get('data') is not None:
            sol = state.get('data')
            s = "Solution:"
            Graphics.draw_text(10, 50, s, Colors.WHITE)
            y = 70
            for i, x in enumerate(sol):
                Graphics.draw_text(10, y, "v{} = {:.4f}".format(i+1, float(x)), Colors.WHITE)
                y += 20
            self.display_matrix(state)

if __name__ == '__main__':
    app = App()
    app.set_scene(MatrixViewer())
    try:
        app.run()
    except Exception as e:
        print("Error: {}".format(e))