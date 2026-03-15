with open('/home/matt3r/calculator-files/gauss_app/src/chemistry.cpp', 'r') as f:
    text = f.read()

import re
old_escape = r'''                if \(sym == SDLK_ESCAPE\) \{
                    if \(state0 == PEROIODIC_TABLE && state1 == ELEMENT_DETAIL\) \{
                        state1 = ELEMENT_LIST;
                    \} else if \(state0 == PEROIODIC_TABLE && state1 == COLOR_OPTION_MENU\) \{
                        state1 = ELEMENT_LIST;
                    \} else if \(state0 == TREND_EXPLORER && state1 == TREND_SCATTER\) \{
                        state1 = TREND_SCATTER_OPTIONS;
                    \} else if \(state0 == TREND_EXPLORER && state1 == TREND_SCATTER_OPTIONS\) \{
                        state1 = TREND_DETAIL;
                    \} else if \(state0 == TREND_EXPLORER && state1 == TREND_DETAIL\) \{
                        if \(detail_scroll_offset > 0\) detail_scroll_offset--;
                    \} else if \(state0 == TREND_EXPLORER && state1 == TREND_DETAIL\) \{
                        if \(detail_scroll_offset < 15\) detail_scroll_offset\+\+;
                    \} else if \(state0 == TREND_EXPLORER && state1 == TREND_HEATMAP\) \{
                        state1 = TREND_DETAIL;
                    \} else if \(state0 == TREND_EXPLORER && state1 == TREND_DETAIL\) \{
                        state1 = TREND_LIST;
                    \} else \{
                        running = false;
                    \}'''

new_escape = '''                if (sym == SDLK_ESCAPE) {
                    if (state0 == PEROIODIC_TABLE && state1 == ELEMENT_DETAIL) {
                        state1 = ELEMENT_LIST;
                    } else if (state0 == PEROIODIC_TABLE && state1 == COLOR_OPTION_MENU) {
                        state1 = ELEMENT_LIST;
                    } else if (state0 == TREND_EXPLORER && state1 == TREND_SCATTER) {
                        state1 = TREND_SCATTER_OPTIONS;
                    } else if (state0 == TREND_EXPLORER && state1 == TREND_SCATTER_OPTIONS) {
                        state1 = TREND_DETAIL;
                    } else if (state0 == TREND_EXPLORER && state1 == TREND_HEATMAP) {
                        state1 = TREND_DETAIL;
                    } else if (state0 == TREND_EXPLORER && state1 == TREND_DETAIL) {
                        state1 = TREND_LIST;
                    } else {
                        running = false;
                    }'''

text = re.sub(old_escape, new_escape, text)

with open('/home/matt3r/calculator-files/gauss_app/src/chemistry.cpp', 'w') as f:
    f.write(text)
