#!/usr/bin/env python3

with open('venv/gauss_elimination_lib.py', 'r') as f:
    gauss = f.read()

with open('tinspire_lib/engine.py', 'r') as f:
    engine = f.read()

with open('tinspire_lib/graphics.py', 'r') as f:
    graphics = f.read()

with open('tinspire_lib/input.py', 'r') as f:
    inp = f.read()

with open('main.py', 'r') as f:
    main_py = f.read()

# Make sure main_py works with the pure python version
import re
main_py = re.sub(r'import sys\n', '', main_py)
main_py = re.sub(r'from tinspire_lib.*?\n', '', main_py)
main_py = re.sub(r'from gauss_elimination_numpy import solve_gauss_jordan_steps\n', '', main_py)

main_py_new = []
for line in main_py.split('\n'):
    if 'self.generator = solve_gauss_jordan_steps(self.matrix)' in line:
        main_py_new.append("        self.all_steps = list(solve_gauss_jordan_steps(self.matrix))")
    elif 'self.steps = [' in line:
        pass
    elif 'next_step = next(self.generator)' in line:
        pass
    elif 'self.steps.append(next_step)' in line:
        pass
    elif 'if self.current_step < len(self.steps) - 1:' in line:
        main_py_new.append("        if self.current_step < len(self.all_steps) - 1:")
    elif 'state = self.steps[self.current_step]' in line:
        main_py_new.append("        state = self.all_steps[self.current_step]")
    elif 'except StopIteration:' in line:
        pass
    elif 'else:' in line and 'next_step' in main_py:
        # this is tricky, just rewrite handle_input
        pass
    else:
        main_py_new.append(line)

main_py_new_str = '\n'.join(main_py_new)
        
# Clean rewrite of handle_input
new_handle_input = """
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
"""
main_py_new_str = re.sub(r'    def handle_input\(self\):.*?    def display_matrix', new_handle_input + '\n    def display_matrix', main_py_new_str, flags=re.DOTALL)


out = f"""
# ==========================
# BUNDLED TI-NSPIRE APP
# ==========================

import ti_draw as dr
import ti_system as ti
import copy

{engine.replace('import ti_draw as dr', '')}
{graphics.replace('import ti_draw as dr', '')}
{inp.replace('import ti_system as ti', '')}

{gauss.replace('import copy', '')}

{main_py_new_str}
"""

with open('bundled_app.py', 'w') as f:
    f.write(out.strip())

print("Bundled strictly into bundled_app.py")
