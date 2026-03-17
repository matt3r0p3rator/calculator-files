
#include "../include/ti_gui.hpp"
#include "../include/chemistry.hpp"
#include "../include/chemistry_data.hpp"
#include "../include/trend_data.hpp"
#include <vector>
#include <string>
#include <cstdio>
#include <cmath>
#include <algorithm>

using namespace std;

enum AppStateP0 {
    PEROIODIC_TABLE,
    TREND_EXPLORER
};

enum AppStateP1 {
    ELEMENT_LIST,
    ELEMENT_DETAIL,
    COLOR_OPTION_MENU,
    TREND_LIST,
    TREND_DETAIL,
    TREND_HEATMAP,
    TREND_SCATTER_OPTIONS,
    TREND_SCATTER
};

enum ColorMode {
    MODE_CATEGORY,
    MODE_BLOCK
};

void ChemistryApp::loadData() {
    elements = PERIODIC_TABLE_DATA;
    trends = TREND_DATA;
}

TIGuiColor getCategoryColor(const string& category) {
    if (category == "Alkali Metal") return COLOR_RED;
    if (category == "Alkaline Earth Metal") return COLOR_ORANGE;
    if (category == "Transition Metal") return COLOR_YELLOW;
    if (category == "Post-transition Metal" || category == "Metal") return COLOR_GREEN;
    if (category == "Metalloid") return COLOR_TEAL;
    if (category == "Nonmetal") return COLOR_CYAN;
    if (category == "Halogen") return COLOR_BLUE;
    if (category == "Noble Gas") return COLOR_PURPLE;
    if (category == "Lanthanide") return COLOR_PINK;
    if (category == "Actinide" || category == "Transactinide") return COLOR_MAGENTA;
    return COLOR_LIGHT_GRAY;
}

TIGuiColor getBlockColor(const string& block) {
    if (block == "s") return COLOR_RED;
    if (block == "p") return COLOR_YELLOW;
    if (block == "d") return COLOR_CYAN;
    if (block == "f") return COLOR_GREEN;
    return COLOR_LIGHT_GRAY;
}

vector<string> wrapText(const string& str, size_t limit) {
    vector<string> res;
    string current;
    string word;
    for (char ch : str) {
        if (ch == ' ' || ch == '\n') {
            if (current.length() + word.length() + 1 > limit) {
                res.push_back(current);
                current = word;
            } else {
                if (!current.empty()) current += " ";
                current += word;
            }
            word = "";
            if (ch == '\n') {
                if (!current.empty()) res.push_back(current);
                current = "";
            }
        } else {
            word += ch;
        }
    }
    if (!word.empty()) {
        if (current.length() + word.length() + 1 > limit) {
            if (!current.empty()) res.push_back(current);
            res.push_back(word);
        } else {
            if (!current.empty()) current += " ";
            current += word;
            res.push_back(current);
        }
    } else if (!current.empty()) {
        res.push_back(current);
    }
    return res;
}

double getTrendValue(const Element& e, const string& field) {
    if (field == "atomic_radius") return e.atomic_radius;
    if (field == "electronegativity") return e.electronegativity;
    if (field == "first_ionization") return e.first_ionization;
    if (field == "melting_point") return e.melting_point;
    if (field == "boiling_point") return e.boiling_point;
    if (field == "specific_heat") return e.specific_heat;
    if (field == "atomic_weight") return e.atomic_weight;
    if (field == "protons" || field == "atomic_number") return e.atomic_number;
    if (field == "neutrons") return e.number_of_neutrons;
    return 0.0;
}

struct HeatColor {
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
    
    // Full spectrum: blue (low) -> cyan -> green -> yellow -> red (high)
    double hue = (1.0 - ratio) * 240.0;
    int hi = (int)(hue / 60.0) % 6;
    double f = (hue / 60.0) - (int)(hue / 60.0);
    Uint8 r, g, b;
    switch(hi) {
        case 0: r=255; g=(Uint8)(f*255); b=0; break;
        case 1: r=(Uint8)((1.0-f)*255); g=255; b=0; break;
        case 2: r=0; g=255; b=(Uint8)(f*255); break;
        case 3: r=0; g=(Uint8)((1.0-f)*255); b=255; break;
        case 4: r=(Uint8)(f*255); g=0; b=255; break;
        default: r=255; g=0; b=(Uint8)((1.0-f)*255); break;
    }
    double brightness = 0.299 * r + 0.587 * g + 0.114 * b;
    bool is_dark = (brightness < 128);
    return {r, g, b, is_dark, false};
}

void ChemistryApp::run(TIGui& gui) {
    AppStateP0 state0 = PEROIODIC_TABLE;
    AppStateP1 state1 = ELEMENT_LIST;
    ColorMode current_color_mode = MODE_CATEGORY;
    int selected_color_menu_idx = 0;
    
    loadData();
    
    int selected_element_idx = 0;
    int detail_scroll_offset = 0;
    int trend_detail_max_scroll = 15;
    int selected_trend_idx = 0;
    int detail_trend_menu_idx = 0; // 0=heatmap, 1=scatterplot
    int scatter_x_trend_idx = 0;
    int scatter_cursor_idx = 0;
    
    // Create spatial grid for better navigation
    int element_grid[9][18];
    for (int r = 0; r < 9; r++)
        for (int c = 0; c < 18; c++)
            element_grid[r][c] = -1;
            
    for (size_t i = 0; i < elements.size(); i++) {
        const Element& e = elements[i];
        int col = -1, row = -1;
        if (e.atomic_number >= 57 && e.atomic_number <= 71) {
            row = 7; col = (e.atomic_number - 57) + 2;
        } else if (e.atomic_number >= 89 && e.atomic_number <= 103) {
            row = 8; col = (e.atomic_number - 89) + 2;
        } else {
            col = (e.group.empty()) ? -1 : atoi(e.group.c_str()) - 1;
            row = (e.period.empty()) ? -1 : atoi(e.period.c_str()) - 1;
        }
        if (col >= 0 && col < 18 && row >= 0 && row < 9) {
            element_grid[row][col] = i;
        }
    }
    
    Uint32 last_key_time = 0;
    
    bool running = true;
    while(running) {
        gui.clear(COLOR_WHITE);
        
        if (state0 == PEROIODIC_TABLE) {
            gui.drawTopBar("Periodic Table        Tab: Trends Explorer");
            if (state1 == ELEMENT_LIST) {
                int start_x = 5;
                int start_y = 25; // Original height to not overlap bottom text
                int cell_w = 17;
                int cell_h = 17;
                
                for (size_t i = 0; i < elements.size(); i++) {
                    const Element& e = elements[i];
                    int col = -1, row = -1;
                    
                    if (e.atomic_number >= 57 && e.atomic_number <= 71) {
                        row = 7;
                        col = (e.atomic_number - 57) + 2;
                    } else if (e.atomic_number >= 89 && e.atomic_number <= 103) {
                        row = 8;
                        col = (e.atomic_number - 89) + 2;
                    } else {
                        // Safe atoi with default to -1 if empty
                        col = (e.group.empty()) ? -1 : atoi(e.group.c_str()) - 1;
                        row = (e.period.empty()) ? -1 : atoi(e.period.c_str()) - 1;
                    }
                    
                    if (col >= 0 && row >= 0) {
                        int x = start_x + col * cell_w;
                        int y = start_y + row * cell_h;
                        
                        TIGuiColor cat_color = COLOR_LIGHT_GRAY;
                        if (current_color_mode == MODE_CATEGORY) cat_color = getCategoryColor(e.category);
                        else if (current_color_mode == MODE_BLOCK) cat_color = getBlockColor(e.block);
                        
                        if (i == (size_t)selected_element_idx && state1 != COLOR_OPTION_MENU) {
                            gui.fillRect(x, y, cell_w, cell_h, COLOR_BLACK);
                            gui.drawText(x + 2, y + 2, e.symbol, COLOR_WHITE);
                            
                            // Bottom info pane
                            string info = "Z=" + to_string(e.atomic_number) + " " + e.name + " (" + e.category + ")";
                            gui.drawText(10, 203, info, COLOR_BLACK);
                            gui.drawText(10, 215, "Weight: " + to_string(e.atomic_weight) + " | Config: " + e.electron_configuration, COLOR_DARK_GRAY);
                        } else {
                            gui.fillRect(x, y, cell_w, cell_h, cat_color);
                            gui.drawRect(x, y, cell_w, cell_h, COLOR_DARK_GRAY);
                            gui.drawText(x + 2, y + 2, e.symbol, COLOR_BLACK);
                        }
                    }
                }
                
                gui.drawBottomBar("Menu:Color | DPad:Move | ENTER:Info | ESC:Exit");
            } else if (state1 == COLOR_OPTION_MENU) {
                gui.drawTopBar("Periodic Table - Color Options");
                
                gui.drawText(20, 40, "Choose Coloring Scheme:", COLOR_BLACK);
                
                string ops[2] = {"1. Color by Category (Metals, Gases, etc.)", "2. Color by Block (s, p, d, f)"};
                for (int i=0; i<2; i++) {
                    if (selected_color_menu_idx == i) {
                        gui.fillRect(15, 65 + i * 20, 280, 20, COLOR_BLUE);
                        gui.drawText(20, 67 + i * 20, ops[i], COLOR_WHITE);
                    } else {
                        gui.drawText(20, 67 + i * 20, ops[i], COLOR_BLACK);
                    }
                }
                
                gui.drawBottomBar(" ^/v: Select | ENTER: OK | ESC: Cancel");
            } else if (state1 == ELEMENT_DETAIL) {
                if (elements.size() > 0) {
                    const Element& e = elements[selected_element_idx];
                    
                    std::vector<std::string> details = {
                        "No: " + to_string(e.atomic_number) + " | Symbol: " + e.symbol,
                        "Name: " + e.name,
                        "Weight: " + to_string(e.atomic_weight),
                        "Category: " + e.category,
                        "Group: " + e.group + " | Period: " + e.period + " | Block: " + e.block,
                        "e- Config: " + e.electron_configuration,
                        "Oxidation States (+): " + e.positive_oxidation_states,
                        "Oxidation States (-): " + e.negative_oxidation_states,
                        "Electronegativity: " + to_string(e.electronegativity),
                        "Density: " + e.density,
                        "Neutrons: " + to_string(e.number_of_neutrons) + " | Protons: " + to_string(e.number_of_protons) + " | e-: " + to_string(e.number_of_electrons),
                        "Atomic Radius: " + to_string(e.atomic_radius) + " pm",
                        "First Ionization: " + to_string(e.first_ionization) + " eV",
                        "Melting Point: " + to_string(e.melting_point) + "K",
                        "Boiling Point: " + to_string(e.boiling_point) + "K",
                        "Isotopes: " + to_string(e.number_of_isotopes),
                        "Discoverer: " + e.discoverer + " (" + e.year + ")",
                        "Specific Heat: " + to_string(e.specific_heat),
                        "Shells: " + to_string(e.number_of_shells) + " | Valence: " + to_string(e.number_of_valence)
                    };
                    
                    int display_lines = 8;
                    for (int i = 0; i < display_lines; i++) {
                        int idx = detail_scroll_offset + i;
                        if (idx < (int)details.size()) {
                            gui.drawText(10, 30 + i * 20, details[idx], COLOR_BLACK);
                        }
                    }
                }
                gui.drawBottomBar(" ^/v: Scroll | ESC/TAB: Back to Table");
            }
        } else if (state0 == TREND_EXPLORER) {
            gui.drawTopBar("Periodic Trends        Tab: Periodic Table");
            if (state1 == TREND_LIST) {
                gui.drawText(20, 30, "Select a property:", COLOR_DARK_GRAY);
                for (size_t i = 0; i < trends.size(); i++) {
                    int y = 60 + i * 20;
                    if (i == (size_t)selected_trend_idx) {
                        gui.fillRect(15, y - 2, 280, 20, COLOR_BLUE);
                        gui.drawText(20, y, trends[i].name, COLOR_WHITE);
                    } else {
                        gui.drawText(20, y, trends[i].name, COLOR_BLACK);
                    }
                }
                gui.drawBottomBar(" ^/v: Select | ENTER: Info | TAB: Table | ESC: Exit");
            } else if (state1 == TREND_DETAIL) {
                const PeriodicTrend& t = trends[selected_trend_idx];
                gui.drawText(20, 30, "Trend: " + t.name, COLOR_DARK_GRAY);
                
                vector<string> b_lines = wrapText("Behavior: " + t.behavior, 45);
                vector<string> d_lines = wrapText("Desc: " + t.description, 45);
                vector<string> w_lines = wrapText("Why: " + t.why, 45);
                vector<string> all_lines;
                for (const auto& l : b_lines) all_lines.push_back(l);
                all_lines.push_back("");
                for (const auto& l : d_lines) all_lines.push_back(l);
                all_lines.push_back("");
                for (const auto& l : w_lines) all_lines.push_back(l);
                
                int display_lines = 7;
                trend_detail_max_scroll = (int)all_lines.size() > display_lines ? (int)all_lines.size() - display_lines : 0;
                
                for (int i = 0; i < display_lines; i++) {
                    int idx = detail_scroll_offset + i;
                    if (idx < (int)all_lines.size()) {
                        gui.drawText(15, 55 + i * 15, all_lines[idx], COLOR_BLACK);
                    }
                }
                
                string opts[2] = {"1. Plot on Heatmap", "2. Compare vs other on Scatterplot"};
                for (int i = 0; i < 2; i++) {
                    if (detail_trend_menu_idx == i) {
                        gui.fillRect(15, 170 + i * 20, 280, 20, COLOR_BLUE);
                        gui.drawText(20, 172 + i * 20, opts[i], COLOR_WHITE);
                    } else {
                        gui.drawText(20, 172 + i * 20, opts[i], COLOR_BLACK);
                    }
                }
                
                gui.drawBottomBar("^/v: Opt | +-/DPad: Scroll | ENT: Go | ESC: Back");
            } else if (state1 == TREND_HEATMAP) {
                const PeriodicTrend& t = trends[selected_trend_idx];
                gui.drawTopBar("Heatmap: " + t.name);
                
                // Find min and max
                double min_val = 999999.0;
                double max_val = -999999.0;
                for (const auto& e : elements) {
                    double v = getTrendValue(e, t.data_field);
                    if (v > 0.0) {
                        if (v < min_val) min_val = v;
                        if (v > max_val) max_val = v;
                    }
                }
                
                int start_x = 5;
                int start_y = 25;
                int cell_w = 17;
                int cell_h = 17;
                for (const auto& e : elements) {
                    int col = -1, row = -1;
                    if (e.atomic_number >= 57 && e.atomic_number <= 71) {
                        row = 7; col = (e.atomic_number - 57) + 2;
                    } else if (e.atomic_number >= 89 && e.atomic_number <= 103) {
                        row = 8; col = (e.atomic_number - 89) + 2;
                    } else {
                        col = (e.group.empty()) ? -1 : atoi(e.group.c_str()) - 1;
                        row = (e.period.empty()) ? -1 : atoi(e.period.c_str()) - 1;
                    }
                    if (col >= 0 && row >= 0) {
                        int x = start_x + col * cell_w;
                        int y = start_y + row * cell_h;
                        double val = getTrendValue(e, t.data_field);
                        HeatColor hc = getSmoothHeatmapColor(val, min_val, max_val);
                        
                        gui.fillRectRGB(x, y, cell_w, cell_h, hc.r, hc.g, hc.b);
                        gui.drawRect(x, y, cell_w, cell_h, COLOR_DARK_GRAY);
                        gui.drawText(x + 2, y + 2, e.symbol, hc.is_dark ? COLOR_WHITE : COLOR_BLACK);
                        if (e.atomic_number == elements[selected_element_idx].atomic_number) {
                            gui.drawRect(x+1, y+1, cell_w-2, cell_h-2, COLOR_WHITE);
                            string info = e.symbol + ": " + to_string(val);
                            gui.drawText(10, 203, info, COLOR_BLACK);
                        }
                    }
                }
                // Legend: full spectrum gradient bar (blue=low -> cyan -> green -> yellow -> red=high)
                gui.drawText(5, 213, "Lo", COLOR_BLACK);
                int bar_x = 24, bar_y = 212, bar_w = 220, bar_h = 10;
                for (int bx = 0; bx < bar_w; bx++) {
                    double br = (double)bx / (bar_w - 1);
                    double bh = (1.0 - br) * 240.0;
                    int bhi = (int)(bh / 60.0) % 6;
                    double bf = (bh / 60.0) - (int)(bh / 60.0);
                    Uint8 lr, lg, lb;
                    switch(bhi) {
                        case 0: lr=255; lg=(Uint8)(bf*255); lb=0; break;
                        case 1: lr=(Uint8)((1.0-bf)*255); lg=255; lb=0; break;
                        case 2: lr=0; lg=255; lb=(Uint8)(bf*255); break;
                        case 3: lr=0; lg=(Uint8)((1.0-bf)*255); lb=255; break;
                        case 4: lr=(Uint8)(bf*255); lg=0; lb=255; break;
                        default: lr=255; lg=0; lb=(Uint8)((1.0-bf)*255); break;
                    }
                    gui.fillRectRGB(bar_x + bx, bar_y, 1, bar_h, lr, lg, lb);
                }
                gui.drawRect(bar_x, bar_y, bar_w, bar_h, COLOR_DARK_GRAY);
                gui.drawText(bar_x + bar_w + 3, 213, "Hi", COLOR_BLACK);
                
                gui.drawBottomBar("DPad: Select | ESC/TAB: Back to Details");
            } else if (state1 == TREND_SCATTER_OPTIONS) {
                gui.drawText(20, 30, "Select X-axis variable:", COLOR_DARK_GRAY);
                for (size_t i = 0; i < trends.size(); i++) {
                    int y = 60 + i * 20;
                    if (i == (size_t)scatter_x_trend_idx) {
                        gui.fillRect(15, y - 2, 280, 20, COLOR_BLUE);
                        gui.drawText(20, y, trends[i].name, COLOR_WHITE);
                    } else {
                        gui.drawText(20, y, trends[i].name, COLOR_BLACK);
                    }
                }
                gui.drawBottomBar(" ^/v: Select | ENTER: Scatterplot | ESC: Back");
            } else if (state1 == TREND_SCATTER) {
                const PeriodicTrend& t_x = trends[scatter_x_trend_idx];
                const PeriodicTrend& t_y = trends[selected_trend_idx];
                gui.drawTopBar("Y:" + t_y.name.substr(0,10) + " X:" + t_x.name.substr(0,10));
                
                double min_x = 999999.0, max_x = -999999.0;
                double min_y = 999999.0, max_y = -999999.0;
                
                struct PData { int e_idx; double vx; double vy; };
                vector<PData> valid_pts;
                
                for (size_t i = 0; i < elements.size(); i++) {
                    const auto& e = elements[i];
                    double vx = getTrendValue(e, t_x.data_field);
                    double vy = getTrendValue(e, t_y.data_field);
                    if (vx > 0.0 && vy > 0.0) {
                        if (vx < min_x) min_x = vx; 
                        if (vx > max_x) max_x = vx;
                        if (vy < min_y) min_y = vy; 
                        if (vy > max_y) max_y = vy;
                        valid_pts.push_back({(int)i, vx, vy});
                    }
                }
                
                sort(valid_pts.begin(), valid_pts.end(), [](const PData& a, const PData& b) {
                    return a.vx < b.vx;
                });
                
                if (scatter_cursor_idx >= (int)valid_pts.size()) {
                    scatter_cursor_idx = (int)valid_pts.size() - 1;
                }
                if (scatter_cursor_idx < 0) scatter_cursor_idx = 0;
                
                int pad_x = 35;
                int pad_y = 40; // From bottom
                int pad_top = 40;
                int g_w = 320 - pad_x - 15;
                int g_h = 240 - pad_top - pad_y - 20; // 240 - pad_top - pad_y - bottombar(20)
                
                // Axes
                gui.fillRect(pad_x, pad_top, 2, 240 - pad_y - 20 - pad_top, COLOR_DARK_GRAY);
                gui.fillRect(pad_x, 240 - pad_y - 20, 320 - 15 - pad_x, 2, COLOR_DARK_GRAY);
                
                gui.drawText(pad_x + 5, 240 - pad_y - 12, t_x.name, COLOR_DARK_GRAY); // X label
                gui.drawText(2, pad_top - 15, t_y.name.substr(0,10), COLOR_DARK_GRAY); // Y label
                
                // Points
                for (int i = 0; i < (int)valid_pts.size(); i++) {
                    const PData& pd = valid_pts[i];
                    int px = pad_x + (int)(((pd.vx - min_x) / (max_x - min_x + 0.0001)) * g_w);
                    int py = 240 - pad_y - 20 - (int)(((pd.vy - min_y) / (max_y - min_y + 0.0001)) * g_h);
                    
                    if (i == scatter_cursor_idx) {
                        gui.fillRect(px - 2, py - 2, 8, 8, COLOR_RED);
                        string info = elements[pd.e_idx].symbol + " (" + to_string(pd.vx) + ", " + to_string(pd.vy) + ")";
                        gui.drawText(2, 240 - 30, info, COLOR_BLACK);
                    } else {
                        gui.fillRect(px, py, 4, 4, COLOR_BLUE);
                    }
                }
                
                gui.drawBottomBar(" <-/->: Scroll Pt | ESC: Back");
            }
        }
        gui.render();
        SDL_Event event;
        if (SDL_WaitEvent(&event)) {
            if (event.type == SDL_KEYDOWN) {
                Uint32 now = SDL_GetTicks();
                // Debounce rapid touchpad inputs when navigating grids (~130ms)
                if ((state0 == PEROIODIC_TABLE && state1 == ELEMENT_LIST) || 
                    (state0 == TREND_EXPLORER && state1 == TREND_HEATMAP)) {
                    if (now - last_key_time < 130) continue;
                    last_key_time = now;
                }
                
                int sym = event.key.keysym.sym;
                if (sym == SDLK_ESCAPE) {
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
                    }
                } else if (sym == SDLK_TAB) {
                    if (state0 == PEROIODIC_TABLE && state1 == ELEMENT_DETAIL) {
                        state1 = ELEMENT_LIST;
                    } else if (state0 == PEROIODIC_TABLE && state1 == COLOR_OPTION_MENU) {
                        state1 = ELEMENT_LIST;
                    } else if (state0 == PEROIODIC_TABLE) {
                        state0 = TREND_EXPLORER;
                        state1 = TREND_LIST;
                    } else if (state0 == TREND_EXPLORER && state1 == TREND_HEATMAP) {
                        state1 = TREND_DETAIL;
                    } else if (state0 == TREND_EXPLORER && (state1 == TREND_SCATTER || state1 == TREND_SCATTER_OPTIONS)) {
                        state1 = TREND_DETAIL;
                    } else if (state0 == TREND_EXPLORER && state1 == TREND_DETAIL) {
                        state1 = TREND_LIST;
                    } else {
                        state0 = PEROIODIC_TABLE;
                        state1 = ELEMENT_LIST;
                    }
                } else if (sym == SDLK_MENU) {
                    if (state0 == PEROIODIC_TABLE && state1 == ELEMENT_LIST) {
                        state1 = COLOR_OPTION_MENU;
                    }
                } else if (sym == SDLK_RETURN || sym == SDLK_KP_ENTER) {
                    if (state0 == PEROIODIC_TABLE && state1 == ELEMENT_LIST) {
                        state1 = ELEMENT_DETAIL;
                        detail_scroll_offset = 0; // Reset scroll on view
                    } else if (state0 == PEROIODIC_TABLE && state1 == COLOR_OPTION_MENU) {
                        current_color_mode = (selected_color_menu_idx == 0) ? MODE_CATEGORY : MODE_BLOCK;
                        state1 = ELEMENT_LIST;
                    } else if (state0 == TREND_EXPLORER && state1 == TREND_LIST) {
                        state1 = TREND_DETAIL;
                        detail_trend_menu_idx = 0;
                        detail_scroll_offset = 0;
                    } else if (state0 == TREND_EXPLORER && state1 == TREND_DETAIL) {
                        if (detail_trend_menu_idx == 0) {
                            state1 = TREND_HEATMAP;
                        } else {
                            state1 = TREND_SCATTER_OPTIONS;
                            scatter_x_trend_idx = 0;
                        }
                    } else if (state0 == TREND_EXPLORER && state1 == TREND_SCATTER_OPTIONS) {
                        state1 = TREND_SCATTER;
                    }
                } else if (sym == SDLK_LEFT || sym == SDLK_KP4) {
                    if (state0 == PEROIODIC_TABLE && state1 == ELEMENT_LIST) {
                        int c = -1, r = -1;
                        for(int rr=0; rr<9; rr++) for(int cc=0; cc<18; cc++) if(element_grid[rr][cc]==selected_element_idx){c=cc; r=rr; break;}
                        if (c >= 0) {
                            int nc = c - 1;
                            while (nc >= 0 && element_grid[r][nc] == -1) nc--;
                            if (nc >= 0 && element_grid[r][nc] != -1) selected_element_idx = element_grid[r][nc];
                            else if (selected_element_idx > 0) selected_element_idx--; // wrap sequentially
                        }
                    } else if (state0 == TREND_EXPLORER && state1 == TREND_DETAIL) {
                        if (detail_scroll_offset > 0) detail_scroll_offset--;
                    } else if (state0 == TREND_EXPLORER && state1 == TREND_HEATMAP) {
                        int c = -1, r = -1;
                        for(int rr=0; rr<9; rr++) for(int cc=0; cc<18; cc++) if(element_grid[rr][cc]==selected_element_idx){c=cc; r=rr; break;}
                        if (c >= 0) {
                            int nc = c - 1;
                            while (nc >= 0 && element_grid[r][nc] == -1) nc--;
                            if (nc >= 0 && element_grid[r][nc] != -1) selected_element_idx = element_grid[r][nc];
                            else if (selected_element_idx > 0) selected_element_idx--; // wrap sequentially
                        }
                    } else if (state0 == TREND_EXPLORER && state1 == TREND_SCATTER) {
                        if (scatter_cursor_idx > 0) scatter_cursor_idx--;
                    }
                } else if (sym == SDLK_RIGHT || sym == SDLK_KP6) {
                    if (state0 == PEROIODIC_TABLE && state1 == ELEMENT_LIST) {
                        int c = -1, r = -1;
                        for(int rr=0; rr<9; rr++) for(int cc=0; cc<18; cc++) if(element_grid[rr][cc]==selected_element_idx){c=cc; r=rr; break;}
                        if (c >= 0) {
                            int nc = c + 1;
                            while (nc < 18 && element_grid[r][nc] == -1) nc++;
                            if (nc < 18 && element_grid[r][nc] != -1) selected_element_idx = element_grid[r][nc];
                            else if (selected_element_idx < (int)elements.size() - 1) selected_element_idx++; // wrap sequentially
                        }
                    } else if (state0 == TREND_EXPLORER && state1 == TREND_DETAIL) {
                        if (detail_scroll_offset < trend_detail_max_scroll) detail_scroll_offset++;
                    } else if (state0 == TREND_EXPLORER && state1 == TREND_HEATMAP) {
                        int c = -1, r = -1;
                        for(int rr=0; rr<9; rr++) for(int cc=0; cc<18; cc++) if(element_grid[rr][cc]==selected_element_idx){c=cc; r=rr; break;}
                        if (c >= 0) {
                            int nc = c + 1;
                            while (nc < 18 && element_grid[r][nc] == -1) nc++;
                            if (nc < 18 && element_grid[r][nc] != -1) selected_element_idx = element_grid[r][nc];
                            else if (selected_element_idx < (int)elements.size() - 1) selected_element_idx++; // wrap sequentially
                        }
                    } else if (state0 == TREND_EXPLORER && state1 == TREND_SCATTER) {
                        scatter_cursor_idx++;
                    }
                } else if (sym == SDLK_UP || sym == SDLK_KP8) {
                    if (state0 == PEROIODIC_TABLE && state1 == ELEMENT_LIST) {
                        int c = -1, r = -1;
                        for(int rr=0; rr<9; rr++) for(int cc=0; cc<18; cc++) if(element_grid[rr][cc]==selected_element_idx){c=cc; r=rr; break;}
                        if (c >= 0) {
                            int nr = r - 1;
                            while (nr >= 0 && element_grid[nr][c] == -1) nr--;
                            if (nr >= 0 && element_grid[nr][c] != -1) selected_element_idx = element_grid[nr][c];
                        }
                    } else if (state0 == PEROIODIC_TABLE && state1 == ELEMENT_DETAIL) {
                        if (detail_scroll_offset > 0) detail_scroll_offset--;
                    } else if (state0 == PEROIODIC_TABLE && state1 == COLOR_OPTION_MENU) {
                        if (selected_color_menu_idx > 0) selected_color_menu_idx--;
                    } else if (state0 == TREND_EXPLORER && state1 == TREND_LIST) {
                        if (selected_trend_idx > 0) selected_trend_idx--;
                    } else if (state0 == TREND_EXPLORER && state1 == TREND_DETAIL) {
                        if (detail_trend_menu_idx > 0) detail_trend_menu_idx--;
                    } else if (state0 == TREND_EXPLORER && state1 == TREND_HEATMAP) {
                        int c = -1, r = -1;
                        for(int rr=0; rr<9; rr++) for(int cc=0; cc<18; cc++) if(element_grid[rr][cc]==selected_element_idx){c=cc; r=rr; break;}
                        if (c >= 0) {
                            int nr = r - 1;
                            while (nr >= 0 && element_grid[nr][c] == -1) nr--;
                            if (nr >= 0 && element_grid[nr][c] != -1) selected_element_idx = element_grid[nr][c];
                        }
                    } else if (state0 == TREND_EXPLORER && state1 == TREND_SCATTER_OPTIONS) {
                        if (scatter_x_trend_idx > 0) scatter_x_trend_idx--;
                    }
                } else if (sym == SDLK_DOWN || sym == SDLK_KP2) {
                    if (state0 == PEROIODIC_TABLE && state1 == ELEMENT_LIST) {
                        int c = -1, r = -1;
                        for(int rr=0; rr<9; rr++) for(int cc=0; cc<18; cc++) if(element_grid[rr][cc]==selected_element_idx){c=cc; r=rr; break;}
                        if (c >= 0) {
                            int nr = r + 1;
                            while (nr < 9 && element_grid[nr][c] == -1) nr++;
                            if (nr < 9 && element_grid[nr][c] != -1) selected_element_idx = element_grid[nr][c];
                        }
                    } else if (state0 == PEROIODIC_TABLE && state1 == ELEMENT_DETAIL) {
                        // Max details items is 17, visible is 8.
                        if (detail_scroll_offset < 17 - 8) detail_scroll_offset++;
                    } else if (state0 == PEROIODIC_TABLE && state1 == COLOR_OPTION_MENU) {
                        if (selected_color_menu_idx < 1) selected_color_menu_idx++;
                    } else if (state0 == TREND_EXPLORER && state1 == TREND_LIST) {
                        if (selected_trend_idx < (int)trends.size() - 1) selected_trend_idx++;
                    } else if (state0 == TREND_EXPLORER && state1 == TREND_DETAIL) {
                        if (detail_trend_menu_idx < 1) detail_trend_menu_idx++;
                    } else if (state0 == TREND_EXPLORER && state1 == TREND_HEATMAP) {
                        int c = -1, r = -1;
                        for(int rr=0; rr<9; rr++) for(int cc=0; cc<18; cc++) if(element_grid[rr][cc]==selected_element_idx){c=cc; r=rr; break;}
                        if (c >= 0) {
                            int nr = r + 1;
                            while (nr < 9 && element_grid[nr][c] == -1) nr++;
                            if (nr < 9 && element_grid[nr][c] != -1) selected_element_idx = element_grid[nr][c];
                        }
                    } else if (state0 == TREND_EXPLORER && state1 == TREND_SCATTER_OPTIONS) {
                        if (scatter_x_trend_idx < (int)trends.size() - 1) scatter_x_trend_idx++;
                    }
                } else if (sym == SDLK_PLUS || sym == SDLK_KP_PLUS) {
                    if (state0 == TREND_EXPLORER && state1 == TREND_DETAIL) {
                        if (detail_scroll_offset < trend_detail_max_scroll) detail_scroll_offset++;
                    }
                } else if (sym == SDLK_MINUS || sym == SDLK_KP_MINUS) {
                    if (state0 == TREND_EXPLORER && state1 == TREND_DETAIL) {
                        if (detail_scroll_offset > 0) detail_scroll_offset--;
                    }
                }
            }
        }
    }
}