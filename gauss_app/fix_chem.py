with open('/home/matt3r/calculator-files/gauss_app/src/chemistry.cpp', 'r') as f:
    text = f.read()

# 1. Fix the rendering of "Why"
old_render = """                  vector<string> b_lines = wrapText("Behavior: " + t.behavior, 45);
                  vector<string> d_lines = wrapText("Desc: " + t.description, 45);
                  vector<string> all_lines;
                  for (const auto& l : b_lines) all_lines.push_back(l);
                  all_lines.push_back("");
                  for (const auto& l : d_lines) all_lines.push_back(l);
                  all_lines.push_back("");
                  all_lines.push_back("Options:");"""

new_render = """                  vector<string> b_lines = wrapText("Behavior: " + t.behavior, 45);
                  vector<string> d_lines = wrapText("Desc: " + t.description, 45);
                  vector<string> w_lines = wrapText("Why: " + t.why, 45);
                  vector<string> all_lines;
                  for (const auto& l : b_lines) all_lines.push_back(l);
                  all_lines.push_back("");
                  for (const auto& l : d_lines) all_lines.push_back(l);
                  all_lines.push_back("");
                  for (const auto& l : w_lines) all_lines.push_back(l);
                  all_lines.push_back("");
                  all_lines.push_back("Options:");"""

text = text.replace(old_render, new_render)

# 2. Add Left scroll
old_left = "} else if (state0 == TREND_EXPLORER && state1 == TREND_HEATMAP) {\n                        int c = -1, r = -1;"
new_left = """} else if (state0 == TREND_EXPLORER && state1 == TREND_DETAIL) {
                        if (detail_scroll_offset > 0) detail_scroll_offset--;
                    } else if (state0 == TREND_EXPLORER && state1 == TREND_HEATMAP) {
                        int c = -1, r = -1;"""
# Using raw replace but taking care since old_left string literal appears twice (for both left and right keys!). 
# Let's be smart. 

with open('/home/matt3r/calculator-files/gauss_app/src/chemistry.cpp', 'w') as f:
    f.write(text)
