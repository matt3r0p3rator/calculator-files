with open('/home/matt3r/calculator-files/gauss_app/include/ti_gui.hpp', 'r') as f:
    text = f.read()

import re
old = r'''    void fillRect\(int x, int y, int w, int h, TIGuiColor color\) \{
      
        SDL_Rect r = \{\(Sint16\)x, \(Sint16\)y, \(Uint16\)w, \(Uint16\)h\};
      
        SDL_FillRect\(screen, &r, mapColor\(color\)\);
      
    \}'''

new = '''    void fillRect(int x, int y, int w, int h, TIGuiColor color) {
        SDL_Rect r = {(Sint16)x, (Sint16)y, (Uint16)w, (Uint16)h};
        SDL_FillRect(screen, &r, mapColor(color));
    }
    
    void fillRectRGB(int x, int y, int w, int h, Uint8 r, Uint8 g, Uint8 b) {
        SDL_Rect rect = {(Sint16)x, (Sint16)y, (Uint16)w, (Uint16)h};
        SDL_FillRect(screen, &rect, SDL_MapRGB(screen->format, r, g, b));
    }'''

text = re.sub(old, new, text)

# Just in case other gaps are there:
text = text.replace('\n      \n', '\n')
text = text.replace('\n    \n', '\n')

with open('/home/matt3r/calculator-files/gauss_app/include/ti_gui.hpp', 'w') as f:
    f.write(text)
