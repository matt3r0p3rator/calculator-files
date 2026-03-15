with open('/home/matt3r/calculator-files/gauss_app/src/chemistry.cpp', 'r') as f:
    text = f.read()

import re

old = r'''                  vector<string> b_lines = wrapText\("Behavior: " \+ t.behavior, 45\);
                  vector<string> d_lines = wrapText\("Desc: " \+ t.description, 45\);
                  vector<string> all_lines;
                  for \(const auto& l : b_lines\) all_lines\.push_back\(l\);
                  all_lines\.push_back\(""\);
                  for \(const auto& l : d_lines\) all_lines\.push_back\(l\);
                  all_lines\.push_back\(""\);
                  all_lines\.push_back\("Options:"\);'''

new = '''                  vector<string> b_lines = wrapText("Behavior: " + t.behavior, 45);
                  vector<string> d_lines = wrapText("Desc: " + t.description, 45);
                  vector<string> w_lines = wrapText("Why: " + t.why, 45);
                  vector<string> all_lines;
                  for (const auto& l : b_lines) all_lines.push_back(l);
                  all_lines.push_back("");
                  for (const auto& l : d_lines) all_lines.push_back(l);
                  all_lines.push_back("");
                  for (const auto& l : w_lines) all_lines.push_back(l);
                  all_lines.push_back("");
                  all_lines.push_back("Options:");'''

text = re.sub(old, new, text)

with open('/home/matt3r/calculator-files/gauss_app/src/chemistry.cpp', 'w') as f:
    f.write(text)
