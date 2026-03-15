with open('/home/matt3r/calculator-files/gauss_app/src/chemistry.cpp', 'r') as f:
    code = f.read()

import re

# replace getHeatmapColor
old_func = r'''TIGuiColor getHeatmapColor\(double val, double min_val, double max_val\) \{
    if \(max_val == min_val \|\| val <= 0.0\) return COLOR_LIGHT_GRAY;
    double ratio = \(val - min_val\) / \(max_val - min_val\);
    
    if \(ratio < 0.2\) return COLOR_BLUE;
    if \(ratio < 0.4\) return COLOR_CYAN;
    if \(ratio < 0.6\) return COLOR_GREEN;
    if \(ratio < 0.8\) return COLOR_YELLOW;
    return COLOR_RED;
\}'''

new_func = '''struct HeatColor {
    Uint8 r, g, b;
    bool is_dark;
    bool is_invalid;
};

HeatColor getSmoothHeatmapColor(double val, double min_val, double max_val) {
    if (max_val == min_val || val <= 0.0) {
        return {220, 220, 220, false, true};
    }
    double ratio = (val - min_val) / (max_val - min_val);
    if (ratio < 0.0) ratio = 0.0;
    if (ratio > 1.0) ratio = 1.0;
    
    Uint8 r, g, b;
    if (ratio <= 0.5) {
        r = (Uint8)(ratio * 2.0 * 255.0);
        g = 255;
        b = 0;
    } else {
        r = 255;
        g = (Uint8)((1.0 - ratio) * 2.0 * 255.0);
        b = 0;
    }
    double brightness = 0.299 * r + 0.587 * g + 0.114 * b;
    bool is_dark = (brightness < 128);
    return {r, g, b, is_dark, false};
}'''

code = re.sub(old_func, new_func, code)

# replace inside TREND_HEATMAP
old_usage = r'''TIGuiColor c = getHeatmapColor\(val, min_val, max_val\);
                        
                        gui\.fillRect\(x, y, cell_w, cell_h, c\);
                        gui\.drawRect\(x, y, cell_w, cell_h, COLOR_DARK_GRAY\);
                        gui\.drawText\(x \+ 2, y \+ 2, e\.symbol, \(c == COLOR_BLUE \|\| c == COLOR_RED\) \? COLOR_WHITE : COLOR_BLACK\);'''

new_usage = '''HeatColor hc = getSmoothHeatmapColor(val, min_val, max_val);
                        
                        gui.fillRectRGB(x, y, cell_w, cell_h, hc.r, hc.g, hc.b);
                        gui.drawRect(x, y, cell_w, cell_h, COLOR_DARK_GRAY);
                        gui.drawText(x + 2, y + 2, e.symbol, hc.is_dark ? COLOR_WHITE : COLOR_BLACK);'''

code = code.replace(old_usage.replace('\\', ''), new_usage)

old_legend = r'''                // Legend
                gui\.drawText\(10, 215, "Low ", COLOR_BLUE\);
                gui\.drawText\(50, 215, "->", COLOR_CYAN\);
                gui\.drawText\(75, 215, "->", COLOR_GREEN\);
                gui\.drawText\(100, 215, "->", COLOR_YELLOW\);
                gui\.drawText\(125, 215, "High", COLOR_RED\);'''

new_legend = '''                // Legend
                gui.drawText(10, 215, "Low ", COLOR_GREEN);
                gui.drawText(50, 215, "->", COLOR_BLACK);
                gui.drawText(75, 215, "->", COLOR_BLACK);
                gui.drawText(100, 215, "->", COLOR_BLACK);
                gui.drawText(125, 215, "High", COLOR_RED);'''

code = re.sub(old_legend, new_legend, code)

with open('/home/matt3r/calculator-files/gauss_app/src/chemistry.cpp', 'w') as f:
    f.write(code)
