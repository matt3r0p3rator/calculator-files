import sys
from tinspire_lib import App, Scene, Input, Keys, Graphics, Colors
from gauss_elimination_numpy import solve_gauss_jordan_steps

class MatrixViewer(Scene):
    def setup(self):
        self.matrix = [
            [2, 1, -1, 8],
            [-3, -1, 2, -11],
            [-2, 1, 2, -3]
        ]
        self.generator = solve_gauss_jordan_steps(self.matrix)
        self.steps = [{'msg': 'Ready. Press RIGHT arrow.', 'matrix': self.matrix}]
        self.current_step = 0
        
    def handle_input(self):
        key = Input.get_key()
        if key == Keys.RIGHT:
            if self.current_step < len(self.steps) - 1:
                self.current_step += 1
            else:
                try:
                    next_step = next(self.generator)
                    self.steps.append(next_step)
                    self.current_step += 1
                except StopIteration:
                    pass
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
                    s_parts.append(f"| {float(x):6.2f}")
                else:
                    s_parts.append(f"{float(x):6.2f}")
            s = "  ".join(s_parts)
            Graphics.draw_text(10, y, f"[ {s} ]", Colors.WHITE)
            y += 20
            
    def draw(self):
        Graphics.fill_screen(Colors.BLACK)
        
        state = self.steps[self.current_step]
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
                Graphics.draw_text(10, y, f"v{i+1} = {float(x):.4f}", Colors.WHITE)
                y += 20
        else:
            self.display_matrix(state)

if __name__ == '__main__':
    app = App()
    app.set_scene(MatrixViewer())
    try:
        app.run()
    except Exception as e:
        print(f"Error: {e}")
