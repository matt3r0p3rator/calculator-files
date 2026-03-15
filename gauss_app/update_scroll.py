with open('/home/matt3r/calculator-files/gauss_app/src/chemistry.cpp', 'r') as f:
    text = f.read()

import re

# Add right arrow to scroll down
old_right = r'''                    } else if \(state0 == TREND_EXPLORER && state1 == TREND_HEATMAP\) \{
                        int c = -1, r = -1;'''
new_right = '''                    } else if (state0 == TREND_EXPLORER && state1 == TREND_DETAIL) {
                        detail_scroll_offset++;
                    } else if (state0 == TREND_EXPLORER && state1 == TREND_HEATMAP) {
                        int c = -1, r = -1;'''

# Add left arrow to scroll up
old_left = r'''                    } else if \(state0 == TREND_EXPLORER && state1 == TREND_HEATMAP\) \{
                        int c = -1, r = -1;'''
new_left = '''                    } else if (state0 == TREND_EXPLORER && state1 == TREND_DETAIL) {
                        if (detail_scroll_offset > 0) detail_scroll_offset--;
                    } else if (state0 == TREND_EXPLORER && state1 == TREND_HEATMAP) {
                        int c = -1, r = -1;'''

# wait, I need to do it precisely.
