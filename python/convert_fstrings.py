import re

with open('bundled_app.py', 'r') as f:
    content = f.read()

# Very basic f-string replacement for the specific strings used in this file
content = content.replace('f"Working on column {col + 1}"', '"Working on column {}".format(col + 1)')
content = content.replace('f"R{col+1} <-> R{swap_row+1}"', '"R{} <-> R{}".format(col+1, swap_row+1)')
content = content.replace('f"R{col+1}  ->  (1 / {pivot_val:.4f}) * R{col+1}"', '"R{}  ->  (1 / {:.4f}) * R{}".format(col+1, pivot_val, col+1)')
content = content.replace('f"R{row+1}  ->  R{row+1}  {sign}  ({abs(factor):.4f} * R{col+1})"', '"R{}  ->  R{}  {}  ({:.4f} * R{})".format(row+1, row+1, sign, abs(factor), col+1)')
content = content.replace('f"| {float(x):6.2f}"', '"| {:6.2f}".format(float(x))')
content = content.replace('f"{float(x):6.2f}"', '"{:6.2f}".format(float(x))')
content = content.replace('f"v{i+1} = {float(x):.4f}"', '"v{} = {:.4f}".format(i+1, float(x))')
content = content.replace('f"[ {s} ]"', '"[ {} ]".format(s)')

with open('bundled_app.py', 'w') as f:
    f.write(content)

print("f-strings converted!")
