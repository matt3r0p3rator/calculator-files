with open('/home/matt3r/calculator-files/gauss_app/src/chemistry.cpp', 'r') as f:
    lines = f.readlines()

new_lines = []
for i, line in enumerate(lines):
    new_lines.append(line)
    
    # Check for Left
    if "} else if (sym == SDLK_LEFT" in line:
        # the next blocks deal with conditions, we can just insert our condition right after the first valid if/else inside
        pass
        
    # We will just find where they are
    if "if (state0 == PEROIODIC_TABLE && state1 == ELEMENT_LIST) {" in line:
        if i-1 >= 0 and ("sym == SDLK_LEFT" in lines[i-1] or "sym == SDLK_RIGHT" in lines[i-1]):
            pass # this is the correct block!

# A more reliable way: regex matching the exact context in the file.
import re
text = "".join(lines)

# Fix detail_scroll_offset init when moving to TREND_DETAIL
old_enter_detail = """                    } else if (state0 == TREND_EXPLORER && state1 == TREND_LIST) {
                        state1 = TREND_DETAIL;
                        detail_trend_menu_idx = 0;
                    } else if"""
new_enter_detail = """                    } else if (state0 == TREND_EXPLORER && state1 == TREND_LIST) {
                        state1 = TREND_DETAIL;
                        detail_trend_menu_idx = 0;
                        detail_scroll_offset = 0;
                    } else if"""
text = text.replace(old_enter_detail, new_enter_detail)

old_left = """                    } else if (state0 == TREND_EXPLORER && state1 == TREND_HEATMAP) {
                        int c = -1, r = -1;
                        for(int rr=0; rr<9; rr++) for(int cc=0; cc<18; cc++) if(element_grid[rr][cc]==selected_element_idx){c=cc; r=rr; break;}
                        if (c >= 0) {
                            int nc = c - 1;"""

new_left = """                    } else if (state0 == TREND_EXPLORER && state1 == TREND_DETAIL) {
                        if (detail_scroll_offset > 0) detail_scroll_offset--;
                    } else if (state0 == TREND_EXPLORER && state1 == TREND_HEATMAP) {
                        int c = -1, r = -1;
                        for(int rr=0; rr<9; rr++) for(int cc=0; cc<18; cc++) if(element_grid[rr][cc]==selected_element_idx){c=cc; r=rr; break;}
                        if (c >= 0) {
                            int nc = c - 1;"""

text = text.replace(old_left, new_left)

old_right = """                    } else if (state0 == TREND_EXPLORER && state1 == TREND_HEATMAP) {
                        int c = -1, r = -1;
                        for(int rr=0; rr<9; rr++) for(int cc=0; cc<18; cc++) if(element_grid[rr][cc]==selected_element_idx){c=cc; r=rr; break;}
                        if (c >= 0) {
                            int nc = c + 1;"""

new_right = """                    } else if (state0 == TREND_EXPLORER && state1 == TREND_DETAIL) {
                        if (detail_scroll_offset < 15) detail_scroll_offset++;
                    } else if (state0 == TREND_EXPLORER && state1 == TREND_HEATMAP) {
                        int c = -1, r = -1;
                        for(int rr=0; rr<9; rr++) for(int cc=0; cc<18; cc++) if(element_grid[rr][cc]==selected_element_idx){c=cc; r=rr; break;}
                        if (c >= 0) {
                            int nc = c + 1;"""

text = text.replace(old_right, new_right)

with open('/home/matt3r/calculator-files/gauss_app/src/chemistry.cpp', 'w') as f:
    f.write(text)
