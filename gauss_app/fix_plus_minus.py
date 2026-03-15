with open('/home/matt3r/calculator-files/gauss_app/src/chemistry.cpp', 'r') as f:
    text = f.read()

import re

# Adding the +/- input key check
# Usually we can add it right before the last closing braces of the event check.
# Let's insert it after the DOWN arrow block.

old_down = r'''                    \} else if \(state0 == TREND_EXPLORER && state1 == TREND_SCATTER_OPTIONS\) \{
                        if \(scatter_x_trend_idx < \(int\)trends.size\(\) - 1\) scatter_x_trend_idx\+\+;
                    \}
                \}'''

new_down = '''                    } else if (state0 == TREND_EXPLORER && state1 == TREND_SCATTER_OPTIONS) {
                        if (scatter_x_trend_idx < (int)trends.size() - 1) scatter_x_trend_idx++;
                    }
                } else if (sym == SDLK_PLUS || sym == SDLK_KP_PLUS) {
                    if (state0 == TREND_EXPLORER && state1 == TREND_DETAIL) {
                        if (detail_scroll_offset < 15) detail_scroll_offset++;
                    }
                } else if (sym == SDLK_MINUS || sym == SDLK_KP_MINUS) {
                    if (state0 == TREND_EXPLORER && state1 == TREND_DETAIL) {
                        if (detail_scroll_offset > 0) detail_scroll_offset--;
                    }
                }'''

text = re.sub(old_down, new_down, text)

# By the way, check if we need to expand the dynamic scroll limit `15` to however large `all_lines.size()` is. Let's fix that while we are here:
# detail_scroll_offset max should really be std::max(0, (int)all_lines.size() - display_lines) but we don't have access to all_lines.size() here
# so checking < 20 or similar is fine for now, 15 is probably good.

with open('/home/matt3r/calculator-files/gauss_app/src/chemistry.cpp', 'w') as f:
    f.write(text)
