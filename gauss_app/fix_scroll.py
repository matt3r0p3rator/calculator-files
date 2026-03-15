with open('/home/matt3r/calculator-files/gauss_app/src/chemistry.cpp', 'r') as f:
    text = f.read()

import re

# Reset detail_scroll_offset when entering TREND_DETAIL
text = re.sub(
    r'''                    \} else if \(state0 == TREND_EXPLORER && state1 == TREND_LIST\) \{\n                        state1 = TREND_DETAIL;\n                        detail_trend_menu_idx = 0;\n''',
    '''                    } else if (state0 == TREND_EXPLORER && state1 == TREND_LIST) {\n                        state1 = TREND_DETAIL;\n                        detail_trend_menu_idx = 0;\n                        detail_scroll_offset = 0;\n''',
    text
)

# Add Left key scroll
text = re.sub(
    r'''                    \} else if \(state0 == TREND_EXPLORER && state1 == TREND_HEATMAP\) \{''',
    '''                    } else if (state0 == TREND_EXPLORER && state1 == TREND_DETAIL) {\n                        if (detail_scroll_offset > 0) detail_scroll_offset--;\n                    } else if (state0 == TREND_EXPLORER && state1 == TREND_HEATMAP) {''',
    text,
    count=1  # Only the first match which is in the LEFT block
)

# Add Right key scroll
text = re.sub(
    r'''                    \} else if \(state0 == TREND_EXPLORER && state1 == TREND_HEATMAP\) \{''',
    '''                    } else if (state0 == TREND_EXPLORER && state1 == TREND_DETAIL) {\n                        if (detail_scroll_offset < 15) detail_scroll_offset++;\n                    } else if (state0 == TREND_EXPLORER && state1 == TREND_HEATMAP) {''',
    text,
    count=1  # Now the first match is in the RIGHT block (since the previous was replaced)
)

with open('/home/matt3r/calculator-files/gauss_app/src/chemistry.cpp', 'w') as f:
    f.write(text)
