with open('/home/matt3r/calculator-files/gauss_app/src/chemistry.cpp', 'r') as f:
    text = f.read()

import re

# Swap Red and Green logic
old_logic = r'''    if \(ratio <= 0.5\) \{
        r = \(Uint8\)\(ratio \* 2.0 \* 255.0\);
        g = 255;
        b = 0;
    \} else \{
        r = 255;
        g = \(Uint8\)\(\(1.0 - ratio\) \* 2.0 \* 255.0\);
        b = 0;
    \}'''

new_logic = '''    if (ratio <= 0.5) {
        r = 255;
        g = (Uint8)(ratio * 2.0 * 255.0);
        b = 0;
    } else {
        r = (Uint8)((1.0 - ratio) * 2.0 * 255.0);
        g = 255;
        b = 0;
    }'''

text = re.sub(old_logic, new_logic, text)

# Update legend visually
old_legend = r'''                // Legend
                gui.drawText\(10, 215, "Low ", COLOR_GREEN\);
                gui.drawText\(50, 215, "->", COLOR_BLACK\);
                gui.drawText\(75, 215, "->", COLOR_BLACK\);
                gui.drawText\(100, 215, "->", COLOR_BLACK\);
                gui.drawText\(125, 215, "High", COLOR_RED\);'''

new_legend = '''                // Legend
                gui.drawText(10, 215, "Low ", COLOR_RED);
                gui.drawText(50, 215, "->", COLOR_BLACK);
                gui.drawText(75, 215, "->", COLOR_BLACK);
                gui.drawText(100, 215, "->", COLOR_BLACK);
                gui.drawText(125, 215, "High", COLOR_GREEN);'''

text = re.sub(old_legend, new_legend, text)

with open('/home/matt3r/calculator-files/gauss_app/src/chemistry.cpp', 'w') as f:
    f.write(text)
