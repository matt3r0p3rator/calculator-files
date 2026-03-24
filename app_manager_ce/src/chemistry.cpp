// chemistry.cpp – TI-84 CE port (STL-free)
#include "../include/ce_gui.hpp"
#include "../include/chemistry.hpp"
#include "../include/chemistry_data.hpp"
#include "../include/trend_data.hpp"
#include "../include/polyatomic_data.hpp"
#include <cstdio>
#include <cmath>
#include <cctype>
#include <cstring>
#include <cstdlib>

// ─── State enums ─────────────────────────────────────────────────────────────
enum AppStateP0 { PEROIODIC_TABLE, TREND_EXPLORER, POLYATOMIC_LIST };
enum AppStateP1 {
    ELEMENT_LIST, ELEMENT_DETAIL, COLOR_OPTION_MENU,
    TREND_LIST, TREND_DETAIL, TREND_HEATMAP,
    TREND_SCATTER_OPTIONS, TREND_SCATTER
};
enum ColorMode  { MODE_CATEGORY, MODE_BLOCK };

// ─── Helpers ─────────────────────────────────────────────────────────────────
static TIGuiColor getCategoryColor(const char* category) {
    if (strcmp(category, "Alkali Metal")         == 0) return COLOR_RED;
    if (strcmp(category, "Alkaline Earth Metal") == 0) return COLOR_ORANGE;
    if (strcmp(category, "Transition Metal")     == 0) return COLOR_YELLOW;
    if (strcmp(category, "Post-transition Metal")== 0 ||
        strcmp(category, "Metal")                == 0) return COLOR_GREEN;
    if (strcmp(category, "Metalloid")            == 0) return COLOR_TEAL;
    if (strcmp(category, "Nonmetal")             == 0) return COLOR_CYAN;
    if (strcmp(category, "Halogen")              == 0) return COLOR_BLUE;
    if (strcmp(category, "Noble Gas")            == 0) return COLOR_PURPLE;
    if (strcmp(category, "Lanthanide")           == 0) return COLOR_PINK;
    if (strcmp(category, "Actinide")             == 0 ||
        strcmp(category, "Transactinide")        == 0) return COLOR_MAGENTA;
    return COLOR_LIGHT_GRAY;
}

static TIGuiColor getBlockColor(const char* block) {
    if (block[0] == 's' && block[1] == '\0') return COLOR_RED;
    if (block[0] == 'p' && block[1] == '\0') return COLOR_YELLOW;
    if (block[0] == 'd' && block[1] == '\0') return COLOR_CYAN;
    if (block[0] == 'f' && block[1] == '\0') return COLOR_GREEN;
    return COLOR_LIGHT_GRAY;
}

// wrapText: splits str into lines of at most limit characters.
// Returns number of lines written into `lines` (max max_lines).
static int wrapText(const char* str, int limit,
                    char lines[][80], int max_lines) {
    int n = 0;
    char cur[80];  cur[0] = '\0';
    char word[80]; word[0] = '\0';
    int ci = 0, wi = 0;

    for (int i = 0; ; i++) {
        char ch = str[i];
        bool end = (ch == '\0');
        if (ch == ' ' || ch == '\n' || end) {
            word[wi] = '\0';
            if (wi > 0) {
                int need = ci + (ci ? 1 : 0) + wi;
                if (need > limit) {
                    if (n < max_lines) {
                        snprintf(lines[n++], 80, "%s", cur);
                    }
                    snprintf(cur, 80, "%s", word);
                    ci = wi;
                } else {
                    if (ci) { cur[ci++] = ' '; }
                    snprintf(cur + ci, 80 - ci, "%s", word);
                    ci = (int)strlen(cur);
                }
            }
            wi = 0;
            if (ch == '\n') {
                if (n < max_lines) snprintf(lines[n++], 80, "%s", cur);
                cur[0] = '\0'; ci = 0;
            }
            if (end) break;
        } else {
            if (wi < 79) word[wi++] = ch;
        }
    }
    if (ci > 0 && n < max_lines) snprintf(lines[n++], 80, "%s", cur);
    return n;
}

static double getTrendValue(const Element& e, const char* field) {
    if (strcmp(field, "atomic_radius")    == 0) return e.atomic_radius;
    if (strcmp(field, "electronegativity")== 0) return e.electronegativity;
    if (strcmp(field, "first_ionization") == 0) return e.first_ionization;
    if (strcmp(field, "melting_point")    == 0) return e.melting_point;
    if (strcmp(field, "boiling_point")    == 0) return e.boiling_point;
    if (strcmp(field, "specific_heat")    == 0) return e.specific_heat;
    if (strcmp(field, "atomic_weight")    == 0) return e.atomic_weight;
    if (strcmp(field, "protons")          == 0 ||
        strcmp(field, "atomic_number")    == 0) return (double)e.atomic_number;
    if (strcmp(field, "neutrons")         == 0) return (double)e.number_of_neutrons;
    return 0.0;
}

struct HeatColor {
    unsigned char r, g, b;
    bool is_dark;
    bool is_invalid;
};

static HeatColor getSmoothHeatmapColor(double val, double min_val, double max_val) {
    if (max_val == min_val || val <= 0.0)
        return {220, 220, 220, false, true};
    double ratio = (val - min_val) / (max_val - min_val);
    if (ratio < 0.0) ratio = 0.0;
    if (ratio > 1.0) ratio = 1.0;
    double hue = (1.0 - ratio) * 240.0;
    int    hi  = (int)(hue / 60.0) % 6;
    double f   = (hue / 60.0) - (int)(hue / 60.0);
    unsigned char r, g, b;
    switch (hi) {
        case 0: r=255; g=(unsigned char)(f*255); b=0;               break;
        case 1: r=(unsigned char)((1.0-f)*255); g=255; b=0;         break;
        case 2: r=0;   g=255; b=(unsigned char)(f*255);             break;
        case 3: r=0;   g=(unsigned char)((1.0-f)*255); b=255;       break;
        case 4: r=(unsigned char)(f*255); g=0; b=255;               break;
        default: r=255; g=0; b=(unsigned char)((1.0-f)*255);        break;
    }
    double brightness = 0.299*r + 0.587*g + 0.114*b;
    return {r, g, b, brightness < 128.0, false};
}

// ─── Main app entry point ─────────────────────────────────────────────────────
void ChemistryApp::run(TIGui& gui) {
    AppStateP0 state0 = PEROIODIC_TABLE;
    AppStateP1 state1 = ELEMENT_LIST;
    ColorMode  current_color_mode      = MODE_CATEGORY;
    int        selected_color_menu_idx = 0;

    int selected_element_idx  = 0;
    int detail_scroll_offset  = 0;
    int trend_detail_max_scroll = 15;
    int selected_trend_idx    = 0;
    int detail_trend_menu_idx = 0;
    int scatter_x_trend_idx   = 0;
    int scatter_cursor_idx    = 0;

    char poly_search[21];   poly_search[0] = '\0';
    int  poly_search_len    = 0;
    int  poly_scroll_offset = 0;
    int  poly_selected_idx  = 0;
    int  poly_name_scroll   = 0;

    // Build spatial grid for arrow-key navigation on the periodic table
    int element_grid[9][18];
    for (int r = 0; r < 9; r++)
        for (int c = 0; c < 18; c++)
            element_grid[r][c] = -1;

    for (int i = 0; i < NUM_ELEMENTS; i++) {
        const Element& e = PERIODIC_TABLE_DATA[i];
        int col = -1, row = -1;
        if (e.atomic_number >= 57 && e.atomic_number <= 71) {
            row = 7; col = (e.atomic_number - 57) + 2;
        } else if (e.atomic_number >= 89 && e.atomic_number <= 103) {
            row = 8; col = (e.atomic_number - 89) + 2;
        } else {
            col = (e.group[0] == '\0')  ? -1 : atoi(e.group)  - 1;
            row = (e.period[0] == '\0') ? -1 : atoi(e.period) - 1;
        }
        if (col >= 0 && col < 18 && row >= 0 && row < 9)
            element_grid[row][col] = i;
    }

    bool running = true;
    while (running) {
        gui.clear(COLOR_WHITE);

        // ══════════════════════════════════════════════════════════════════════
        // PERIODIC TABLE tab
        // ══════════════════════════════════════════════════════════════════════
        if (state0 == PEROIODIC_TABLE) {
            gui.drawTopBar("Periodic Table        2nd: Trends Explorer");

            if (state1 == ELEMENT_LIST) {
                const int start_x = 5, start_y = 25;
                const int cell_w  = 17, cell_h  = 17;

                for (int i = 0; i < NUM_ELEMENTS; i++) {
                    const Element& e = PERIODIC_TABLE_DATA[i];
                    int col = -1, row = -1;
                    if (e.atomic_number >= 57 && e.atomic_number <= 71) {
                        row = 7; col = (e.atomic_number - 57) + 2;
                    } else if (e.atomic_number >= 89 && e.atomic_number <= 103) {
                        row = 8; col = (e.atomic_number - 89) + 2;
                    } else {
                        col = (e.group[0] == '\0')  ? -1 : atoi(e.group)  - 1;
                        row = (e.period[0] == '\0') ? -1 : atoi(e.period) - 1;
                    }
                    if (col >= 0 && row >= 0) {
                        int x = start_x + col * cell_w;
                        int y = start_y + row * cell_h;
                        TIGuiColor cat_color = COLOR_LIGHT_GRAY;
                        if (current_color_mode == MODE_CATEGORY)
                            cat_color = getCategoryColor(e.category);
                        else if (current_color_mode == MODE_BLOCK)
                            cat_color = getBlockColor(e.block);

                        if (i == selected_element_idx &&
                            state1 != COLOR_OPTION_MENU) {
                            gui.fillRect(x, y, cell_w, cell_h, COLOR_BLACK);
                            gui.drawText(x + 2, y + 2, e.symbol, COLOR_WHITE);
                            char info[80];
                            snprintf(info, sizeof(info), "Z=%d %s (%s)",
                                     e.atomic_number, e.name, e.category);
                            gui.drawText(10, 203, info, COLOR_BLACK);
                            char cfg[80];
                            snprintf(cfg, sizeof(cfg), "Wt:%.4g | %s",
                                     e.atomic_weight, e.electron_configuration);
                            gui.drawText(10, 215, cfg, COLOR_DARK_GRAY);
                        } else {
                            gui.fillRect(x, y, cell_w, cell_h, cat_color);
                            gui.drawRect(x, y, cell_w, cell_h, COLOR_DARK_GRAY);
                            gui.drawText(x + 2, y + 2, e.symbol, COLOR_BLACK);
                        }
                    }
                }
                gui.drawBottomBar("MODE:Color | DPad:Move | ENTER:Info | CLR:Exit");

            } else if (state1 == COLOR_OPTION_MENU) {
                gui.drawTopBar("Periodic Table - Colour Options");
                gui.drawText(20, 40, "Choose Coloring Scheme:", COLOR_BLACK);
                const char* ops[2] = {
                    "1. Color by Category (Metals, Gases...)",
                    "2. Color by Block (s, p, d, f)"
                };
                for (int i = 0; i < 2; i++) {
                    if (selected_color_menu_idx == i) {
                        gui.fillRect(15, 65 + i * 20, 280, 20, COLOR_BLUE);
                        gui.drawText(20, 67 + i * 20, ops[i], COLOR_WHITE);
                    } else {
                        gui.drawText(20, 67 + i * 20, ops[i], COLOR_BLACK);
                    }
                }
                gui.drawBottomBar(" ^/v:Select | ENTER:OK | CLR:Cancel");

            } else if (state1 == ELEMENT_DETAIL) {
                const Element& e = PERIODIC_TABLE_DATA[selected_element_idx];

                char det[19][80];
                int  n_det = 0;
                snprintf(det[n_det++], 80, "No: %d | Symbol: %s",
                         e.atomic_number, e.symbol);
                snprintf(det[n_det++], 80, "Name: %s", e.name);
                snprintf(det[n_det++], 80, "Weight: %.4g", e.atomic_weight);
                snprintf(det[n_det++], 80, "Category: %s", e.category);
                snprintf(det[n_det++], 80, "Group: %s | Period: %s | Block: %s",
                         e.group, e.period, e.block);
                snprintf(det[n_det++], 80, "e- Config: %s",
                         e.electron_configuration);
                snprintf(det[n_det++], 80, "Oxidation (+): %s",
                         e.positive_oxidation_states);
                snprintf(det[n_det++], 80, "Oxidation (-): %s",
                         e.negative_oxidation_states);
                snprintf(det[n_det++], 80, "Electronegativity: %.3g",
                         e.electronegativity);
                snprintf(det[n_det++], 80, "Density: %s", e.density);
                snprintf(det[n_det++], 80, "Neutrons:%d Protons:%d e-:%d",
                         e.number_of_neutrons, e.number_of_protons,
                         e.number_of_electrons);
                snprintf(det[n_det++], 80, "Atomic Radius: %.4g pm",
                         e.atomic_radius);
                snprintf(det[n_det++], 80, "First Ionization: %.4g eV",
                         e.first_ionization);
                snprintf(det[n_det++], 80, "Melting Point: %.4g K",
                         e.melting_point);
                snprintf(det[n_det++], 80, "Boiling Point: %.4g K",
                         e.boiling_point);
                snprintf(det[n_det++], 80, "Isotopes: %d",
                         e.number_of_isotopes);
                snprintf(det[n_det++], 80, "Discoverer: %s (%s)",
                         e.discoverer, e.year);
                snprintf(det[n_det++], 80, "Specific Heat: %.4g",
                         e.specific_heat);
                snprintf(det[n_det++], 80, "Shells: %d | Valence: %d",
                         e.number_of_shells, e.number_of_valence);

                const int display_lines = 8;
                for (int i = 0; i < display_lines; i++) {
                    int idx = detail_scroll_offset + i;
                    if (idx < n_det)
                        gui.drawText(10, 30 + i * 20, det[idx], COLOR_BLACK);
                }
                gui.drawBottomBar(" ^/v:Scroll | CLR/2nd:Back to Table");
            }

        // ══════════════════════════════════════════════════════════════════════
        // TREND EXPLORER tab
        // ══════════════════════════════════════════════════════════════════════
        } else if (state0 == TREND_EXPLORER) {
            gui.drawTopBar("Periodic Trends        2nd: Polyatomics");

            if (state1 == TREND_LIST) {
                gui.drawText(20, 30, "Select a property:", COLOR_DARK_GRAY);
                for (int i = 0; i < NUM_TRENDS; i++) {
                    int y = 60 + i * 20;
                    if (i == selected_trend_idx) {
                        gui.fillRect(15, y - 2, 280, 20, COLOR_BLUE);
                        gui.drawText(20, y, TREND_DATA[i].name, COLOR_WHITE);
                    } else {
                        gui.drawText(20, y, TREND_DATA[i].name, COLOR_BLACK);
                    }
                }
                gui.drawBottomBar(" ^/v:Select | ENTER:Info | 2nd:Table | CLR:Exit");

            } else if (state1 == TREND_DETAIL) {
                const PeriodicTrend& t = TREND_DATA[selected_trend_idx];

                char title_buf[60];
                snprintf(title_buf, sizeof(title_buf), "Trend: %s", t.name);
                gui.drawText(20, 30, title_buf, COLOR_DARK_GRAY);

                char all_lines[40][80];
                int  n_all = 0;

                char beh_prefix[60];
                snprintf(beh_prefix, sizeof(beh_prefix), "Behavior: %s", t.behavior);
                n_all += wrapText(beh_prefix, 45, all_lines + n_all, 40 - n_all);
                if (n_all < 40) all_lines[n_all++][0] = '\0';

                char desc_prefix[300];
                snprintf(desc_prefix, sizeof(desc_prefix), "Desc: %s", t.description);
                n_all += wrapText(desc_prefix, 45, all_lines + n_all, 40 - n_all);
                if (n_all < 40) all_lines[n_all++][0] = '\0';

                char why_prefix[300];
                snprintf(why_prefix, sizeof(why_prefix), "Why: %s", t.why);
                n_all += wrapText(why_prefix, 45, all_lines + n_all, 40 - n_all);

                const int display_lines = 7;
                trend_detail_max_scroll = (n_all > display_lines)
                    ? n_all - display_lines : 0;

                for (int i = 0; i < display_lines; i++) {
                    int idx = detail_scroll_offset + i;
                    if (idx < n_all)
                        gui.drawText(15, 55 + i * 15, all_lines[idx], COLOR_BLACK);
                }

                const char* opts[2] = {
                    "1. Plot on Heatmap",
                    "2. Compare vs other on Scatterplot"
                };
                for (int i = 0; i < 2; i++) {
                    if (detail_trend_menu_idx == i) {
                        gui.fillRect(15, 170 + i * 20, 280, 20, COLOR_BLUE);
                        gui.drawText(20, 172 + i * 20, opts[i], COLOR_WHITE);
                    } else {
                        gui.drawText(20, 172 + i * 20, opts[i], COLOR_BLACK);
                    }
                }
                gui.drawBottomBar("^/v:Opt | </> :TextScroll | ENTER:Go | CLR:Back");

            } else if (state1 == TREND_HEATMAP) {
                const PeriodicTrend& t = TREND_DATA[selected_trend_idx];
                char top[40];
                snprintf(top, sizeof(top), "Heatmap: %.30s", t.name);
                gui.drawTopBar(top);

                double min_val =  999999.0, max_val = -999999.0;
                for (int i = 0; i < NUM_ELEMENTS; i++) {
                    double v = getTrendValue(PERIODIC_TABLE_DATA[i], t.data_field);
                    if (v > 0.0) {
                        if (v < min_val) min_val = v;
                        if (v > max_val) max_val = v;
                    }
                }

                const int start_x = 5, start_y = 25;
                const int cell_w  = 17, cell_h  = 17;
                for (int i = 0; i < NUM_ELEMENTS; i++) {
                    const Element& e = PERIODIC_TABLE_DATA[i];
                    int col = -1, row = -1;
                    if (e.atomic_number >= 57 && e.atomic_number <= 71) {
                        row = 7; col = (e.atomic_number - 57) + 2;
                    } else if (e.atomic_number >= 89 && e.atomic_number <= 103) {
                        row = 8; col = (e.atomic_number - 89) + 2;
                    } else {
                        col = (e.group[0] == '\0')  ? -1 : atoi(e.group)  - 1;
                        row = (e.period[0] == '\0') ? -1 : atoi(e.period) - 1;
                    }
                    if (col >= 0 && row >= 0) {
                        int x = start_x + col * cell_w;
                        int y = start_y + row * cell_h;
                        double val = getTrendValue(e, t.data_field);
                        HeatColor hc = getSmoothHeatmapColor(val, min_val, max_val);
                        gui.fillRectRGB(x, y, cell_w, cell_h, hc.r, hc.g, hc.b);
                        gui.drawRect(x, y, cell_w, cell_h, COLOR_DARK_GRAY);
                        gui.drawText(x + 2, y + 2, e.symbol,
                                     hc.is_dark ? COLOR_WHITE : COLOR_BLACK);
                        if (i == selected_element_idx) {
                            gui.drawRect(x+1, y+1, cell_w-2, cell_h-2, COLOR_WHITE);
                            char info[40];
                            snprintf(info, sizeof(info), "%s: %.4g", e.symbol, val);
                            gui.drawText(10, 203, info, COLOR_BLACK);
                        }
                    }
                }

                // Colour legend bar
                gui.drawText(5, 213, "Lo", COLOR_BLACK);
                const int bx = 24, by = 212, bw = 220, bh = 10;
                for (int i = 0; i < bw; i++) {
                    double br = (double)i / (bw - 1);
                    double bhue = (1.0 - br) * 240.0;
                    int bhi = (int)(bhue / 60.0) % 6;
                    double bf = (bhue / 60.0) - (int)(bhue / 60.0);
                    unsigned char lr, lg, lb;
                    switch (bhi) {
                        case 0: lr=255; lg=(unsigned char)(bf*255); lb=0;              break;
                        case 1: lr=(unsigned char)((1.0-bf)*255); lg=255; lb=0;        break;
                        case 2: lr=0; lg=255; lb=(unsigned char)(bf*255);              break;
                        case 3: lr=0; lg=(unsigned char)((1.0-bf)*255); lb=255;        break;
                        case 4: lr=(unsigned char)(bf*255); lg=0; lb=255;              break;
                        default: lr=255; lg=0; lb=(unsigned char)((1.0-bf)*255);       break;
                    }
                    gui.fillRectRGB(bx + i, by, 1, bh, lr, lg, lb);
                }
                gui.drawRect(bx, by, bw, bh, COLOR_DARK_GRAY);
                gui.drawText(bx + bw + 3, 213, "Hi", COLOR_BLACK);
                gui.drawBottomBar("DPad:Select | CLR/2nd:Back to Details");

            } else if (state1 == TREND_SCATTER_OPTIONS) {
                gui.drawText(20, 30, "Select X-axis variable:", COLOR_DARK_GRAY);
                for (int i = 0; i < NUM_TRENDS; i++) {
                    int y = 60 + i * 20;
                    if (i == scatter_x_trend_idx) {
                        gui.fillRect(15, y - 2, 280, 20, COLOR_BLUE);
                        gui.drawText(20, y, TREND_DATA[i].name, COLOR_WHITE);
                    } else {
                        gui.drawText(20, y, TREND_DATA[i].name, COLOR_BLACK);
                    }
                }
                gui.drawBottomBar(" ^/v:Select | ENTER:Scatterplot | CLR:Back");

            } else if (state1 == TREND_SCATTER) {
                const PeriodicTrend& t_x = TREND_DATA[scatter_x_trend_idx];
                const PeriodicTrend& t_y = TREND_DATA[selected_trend_idx];

                char top[40];
                char ybuf[11], xbuf[11];
                snprintf(ybuf, sizeof(ybuf), "%s", t_y.name);
                snprintf(xbuf, sizeof(xbuf), "%s", t_x.name);
                snprintf(top, sizeof(top), "Y:%.10s X:%.10s", ybuf, xbuf);
                gui.drawTopBar(top);

                double min_x =  999999.0, max_x = -999999.0;
                double min_y =  999999.0, max_y = -999999.0;
                struct PData { int e_idx; double vx, vy; };
                static PData valid_pts[118];
                int n_valid = 0;

                for (int i = 0; i < NUM_ELEMENTS; i++) {
                    const Element& e = PERIODIC_TABLE_DATA[i];
                    double vx = getTrendValue(e, t_x.data_field);
                    double vy = getTrendValue(e, t_y.data_field);
                    if (vx > 0.0 && vy > 0.0) {
                        if (vx < min_x) min_x = vx;
                        if (vx > max_x) max_x = vx;
                        if (vy < min_y) min_y = vy;
                        if (vy > max_y) max_y = vy;
                        valid_pts[n_valid++] = {i, vx, vy};
                    }
                }
                // Simple insertion sort by vx (n<=118, fast enough)
                for (int i = 1; i < n_valid; i++) {
                    PData key = valid_pts[i];
                    int j = i - 1;
                    while (j >= 0 && valid_pts[j].vx > key.vx) {
                        valid_pts[j + 1] = valid_pts[j]; j--;
                    }
                    valid_pts[j + 1] = key;
                }

                if (scatter_cursor_idx >= n_valid) scatter_cursor_idx = n_valid - 1;
                if (scatter_cursor_idx < 0) scatter_cursor_idx = 0;

                const int pad_x = 35, pad_y = 40, pad_top = 40;
                const int g_w = 320 - pad_x - 15;
                const int g_h = 240 - pad_top - pad_y - 20;

                gui.fillRect(pad_x, pad_top, 2, 240 - pad_y - 20 - pad_top, COLOR_DARK_GRAY);
                gui.fillRect(pad_x, 240 - pad_y - 20, 320 - 15 - pad_x, 2, COLOR_DARK_GRAY);
                gui.drawText(pad_x + 5, 240 - pad_y - 12, t_x.name, COLOR_DARK_GRAY);
                char yl[11]; snprintf(yl, sizeof(yl), "%s", t_y.name);
                gui.drawText(2, pad_top - 15, yl, COLOR_DARK_GRAY);

                for (int i = 0; i < n_valid; i++) {
                    const PData& pd = valid_pts[i];
                    int px = pad_x + (int)(((pd.vx - min_x) / (max_x - min_x + 0.0001)) * g_w);
                    int py = 240 - pad_y - 20 -
                             (int)(((pd.vy - min_y) / (max_y - min_y + 0.0001)) * g_h);
                    if (i == scatter_cursor_idx) {
                        gui.fillRect(px - 2, py - 2, 8, 8, COLOR_RED);
                        char info[60];
                        snprintf(info, sizeof(info), "%s (%.4g, %.4g)",
                                 PERIODIC_TABLE_DATA[pd.e_idx].symbol, pd.vx, pd.vy);
                        gui.drawText(2, 240 - 30, info, COLOR_BLACK);
                    } else {
                        gui.fillRect(px, py, 4, 4, COLOR_BLUE);
                    }
                }
                gui.drawBottomBar(" </> : Scroll Pt | CLR: Back");
            }

        // ══════════════════════════════════════════════════════════════════════
        // POLYATOMIC IONS tab
        // ══════════════════════════════════════════════════════════════════════
        } else if (state0 == POLYATOMIC_LIST) {
            gui.drawTopBar("Polyatomic Ions      2nd: Periodic Table");

            // Build filtered list
            int filtered[44];
            int n_filtered = 0;
            // Lower-case query copy
            char query_lower[21];
            strncpy(query_lower, poly_search, 20);
            query_lower[20] = '\0';
            for (int k = 0; query_lower[k]; k++)
                query_lower[k] = (char)tolower((unsigned char)query_lower[k]);

            for (int i = 0; i < NUM_POLYATOMICS && n_filtered < 44; i++) {
                // lower-case copies of name and formula
                char nl[80], fl[40];
                int len;
                len = (int)strlen(POLYATOMIC_DATA[i].name);
                if (len > 79) len = 79;
                for (int k = 0; k < len; k++)
                    nl[k] = (char)tolower((unsigned char)POLYATOMIC_DATA[i].name[k]);
                nl[len] = '\0';
                len = (int)strlen(POLYATOMIC_DATA[i].formula);
                if (len > 39) len = 39;
                for (int k = 0; k < len; k++)
                    fl[k] = (char)tolower((unsigned char)POLYATOMIC_DATA[i].formula[k]);
                fl[len] = '\0';

                if (query_lower[0] == '\0' ||
                    strstr(nl, query_lower) != NULL ||
                    strstr(fl, query_lower) != NULL)
                    filtered[n_filtered++] = i;
            }

            if (n_filtered > 0 && poly_selected_idx >= n_filtered)
                poly_selected_idx = n_filtered - 1;
            if (poly_selected_idx < 0) poly_selected_idx = 0;

            const int POLY_VISIBLE = 8;
            if (poly_selected_idx < poly_scroll_offset)
                poly_scroll_offset = poly_selected_idx;
            if (poly_selected_idx >= poly_scroll_offset + POLY_VISIBLE)
                poly_scroll_offset = poly_selected_idx - POLY_VISIBLE + 1;
            if (poly_scroll_offset < 0) poly_scroll_offset = 0;

            // Search bar
            gui.fillRect(5, 23, 310, 14, COLOR_LIGHT_GRAY);
            gui.drawRect(5, 23, 310, 14, COLOR_DARK_GRAY);
            char search_prompt[50];
            if (gui.isAlphaMode())
                snprintf(search_prompt, sizeof(search_prompt),
                         "Search[ALPHA]: %s_", poly_search);
            else
                snprintf(search_prompt, sizeof(search_prompt),
                         "Search: %s_", poly_search);
            gui.drawText(8, 25, search_prompt, COLOR_BLACK);

            gui.drawText(10,  39, "Name",    COLOR_DARK_GRAY);
            gui.drawText(195, 39, "Formula", COLOR_DARK_GRAY);
            gui.drawText(278, 39, "Chg",     COLOR_DARK_GRAY);

            for (int i = 0; i < POLY_VISIBLE; i++) {
                int fi = poly_scroll_offset + i;
                if (fi >= n_filtered) break;
                int pi = filtered[fi];
                int y  = 49 + i * 20;

                char charge_str[8];
                if (POLYATOMIC_DATA[pi].charge > 0)
                    snprintf(charge_str, sizeof(charge_str), "+%d",
                             POLYATOMIC_DATA[pi].charge);
                else
                    snprintf(charge_str, sizeof(charge_str), "%d",
                             POLYATOMIC_DATA[pi].charge);

                const char* full_name = POLYATOMIC_DATA[pi].name;
                int full_len = (int)strlen(full_name);
                char disp_name[30];
                if (fi == poly_selected_idx) {
                    int ns = poly_name_scroll;
                    if (ns >= full_len) ns = full_len > 0 ? full_len - 1 : 0;
                    const char* scrolled = full_name + ns;
                    int slen = (int)strlen(scrolled);
                    bool has_left  = ns > 0;
                    bool has_right = slen > 24;
                    if (has_right)
                        snprintf(disp_name, sizeof(disp_name), "%.23s>", scrolled);
                    else
                        snprintf(disp_name, sizeof(disp_name), "%s", scrolled);
                    if (has_left && disp_name[0]) disp_name[0] = '<';
                } else {
                    if (full_len > 24)
                        snprintf(disp_name, sizeof(disp_name), "%.23s>", full_name);
                    else
                        snprintf(disp_name, sizeof(disp_name), "%s", full_name);
                }

                if (fi == poly_selected_idx) {
                    gui.fillRect(5, y - 1, 310, 18, COLOR_BLUE);
                    gui.drawText(10,  y + 1, disp_name,                     COLOR_WHITE);
                    gui.drawText(195, y + 1, POLYATOMIC_DATA[pi].formula,   COLOR_WHITE);
                    gui.drawText(278, y + 1, charge_str,                    COLOR_WHITE);
                } else {
                    gui.drawText(10,  y + 1, disp_name,                     COLOR_BLACK);
                    gui.drawText(195, y + 1, POLYATOMIC_DATA[pi].formula,   COLOR_DARK_GRAY);
                    gui.drawText(278, y + 1, charge_str,                    COLOR_DARK_GRAY);
                }
            }

            char count_buf[32];
            snprintf(count_buf, sizeof(count_buf), "%d/%d ions",
                     n_filtered, NUM_POLYATOMICS);
            gui.drawText(5, 214, count_buf, COLOR_DARK_GRAY);
            gui.drawBottomBar("ALPHA+key:Search | DEL:Clear | DPad:Scroll | 2nd:Switch");
        }

        // ── Render frame ──────────────────────────────────────────────────────
        gui.render();

        // ── Input handling ────────────────────────────────────────────────────
        int key = gui.waitKey();

        // ── ESC / Clear ───────────────────────────────────────────────────────
        if (key == KEY_ESCAPE) {
            if (state0 == POLYATOMIC_LIST) {
                running = false;
            } else if (state0 == PEROIODIC_TABLE && state1 == ELEMENT_DETAIL) {
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

        // ── TAB (2nd) – cycle tabs ────────────────────────────────────────────
        } else if (key == KEY_TAB) {
            if (state0 == PEROIODIC_TABLE && state1 == ELEMENT_DETAIL) {
                state1 = ELEMENT_LIST;
            } else if (state0 == PEROIODIC_TABLE && state1 == COLOR_OPTION_MENU) {
                state1 = ELEMENT_LIST;
            } else if (state0 == PEROIODIC_TABLE) {
                state0 = TREND_EXPLORER; state1 = TREND_LIST;
            } else if (state0 == TREND_EXPLORER && state1 == TREND_HEATMAP) {
                state1 = TREND_DETAIL;
            } else if (state0 == TREND_EXPLORER &&
                       (state1 == TREND_SCATTER || state1 == TREND_SCATTER_OPTIONS)) {
                state1 = TREND_DETAIL;
            } else if (state0 == TREND_EXPLORER && state1 == TREND_DETAIL) {
                state1 = TREND_LIST;
            } else if (state0 == TREND_EXPLORER) {
                state0 = POLYATOMIC_LIST;
                poly_search[0] = '\0'; poly_search_len = 0;
                poly_scroll_offset = 0; poly_selected_idx = 0;
            } else if (state0 == POLYATOMIC_LIST) {
                state0 = PEROIODIC_TABLE; state1 = ELEMENT_LIST;
            } else {
                state0 = PEROIODIC_TABLE; state1 = ELEMENT_LIST;
            }

        // ── Mode key – open colour menu ───────────────────────────────────────
        } else if (key == KEY_MENU) {
            if (state0 == PEROIODIC_TABLE && state1 == ELEMENT_LIST)
                state1 = COLOR_OPTION_MENU;

        // ── Enter ─────────────────────────────────────────────────────────────
        } else if (key == KEY_ENTER) {
            if (state0 == PEROIODIC_TABLE && state1 == ELEMENT_LIST) {
                state1 = ELEMENT_DETAIL; detail_scroll_offset = 0;
            } else if (state0 == PEROIODIC_TABLE && state1 == COLOR_OPTION_MENU) {
                current_color_mode = (selected_color_menu_idx == 0)
                    ? MODE_CATEGORY : MODE_BLOCK;
                state1 = ELEMENT_LIST;
            } else if (state0 == TREND_EXPLORER && state1 == TREND_LIST) {
                state1 = TREND_DETAIL;
                detail_trend_menu_idx = 0; detail_scroll_offset = 0;
            } else if (state0 == TREND_EXPLORER && state1 == TREND_DETAIL) {
                if (detail_trend_menu_idx == 0) {
                    state1 = TREND_HEATMAP;
                } else {
                    state1 = TREND_SCATTER_OPTIONS; scatter_x_trend_idx = 0;
                }
            } else if (state0 == TREND_EXPLORER && state1 == TREND_SCATTER_OPTIONS) {
                state1 = TREND_SCATTER;
            }

        // ── Left ──────────────────────────────────────────────────────────────
        } else if (key == KEY_LEFT) {
            if (state0 == POLYATOMIC_LIST) {
                if (poly_name_scroll > 0) poly_name_scroll--;
            } else if (state0 == PEROIODIC_TABLE && state1 == ELEMENT_LIST) {
                int c = -1, r = -1;
                for (int rr = 0; rr < 9; rr++) for (int cc = 0; cc < 18; cc++)
                    if (element_grid[rr][cc] == selected_element_idx) { c=cc; r=rr; break; }
                if (c >= 0) {
                    int nc = c - 1;
                    while (nc >= 0 && element_grid[r][nc] == -1) nc--;
                    if (nc >= 0) selected_element_idx = element_grid[r][nc];
                    else if (selected_element_idx > 0) selected_element_idx--;
                }
            } else if (state0 == TREND_EXPLORER && state1 == TREND_DETAIL) {
                if (detail_scroll_offset > 0) detail_scroll_offset--;
            } else if (state0 == TREND_EXPLORER && state1 == TREND_HEATMAP) {
                int c = -1, r = -1;
                for (int rr = 0; rr < 9; rr++) for (int cc = 0; cc < 18; cc++)
                    if (element_grid[rr][cc] == selected_element_idx) { c=cc; r=rr; break; }
                if (c >= 0) {
                    int nc = c - 1;
                    while (nc >= 0 && element_grid[r][nc] == -1) nc--;
                    if (nc >= 0) selected_element_idx = element_grid[r][nc];
                    else if (selected_element_idx > 0) selected_element_idx--;
                }
            } else if (state0 == TREND_EXPLORER && state1 == TREND_SCATTER) {
                if (scatter_cursor_idx > 0) scatter_cursor_idx--;
            }

        // ── Right ─────────────────────────────────────────────────────────────
        } else if (key == KEY_RIGHT) {
            if (state0 == POLYATOMIC_LIST) {
                poly_name_scroll++;
            } else if (state0 == PEROIODIC_TABLE && state1 == ELEMENT_LIST) {
                int c = -1, r = -1;
                for (int rr = 0; rr < 9; rr++) for (int cc = 0; cc < 18; cc++)
                    if (element_grid[rr][cc] == selected_element_idx) { c=cc; r=rr; break; }
                if (c >= 0) {
                    int nc = c + 1;
                    while (nc < 18 && element_grid[r][nc] == -1) nc++;
                    if (nc < 18) selected_element_idx = element_grid[r][nc];
                    else if (selected_element_idx < NUM_ELEMENTS - 1)
                        selected_element_idx++;
                }
            } else if (state0 == TREND_EXPLORER && state1 == TREND_DETAIL) {
                if (detail_scroll_offset < trend_detail_max_scroll) detail_scroll_offset++;
            } else if (state0 == TREND_EXPLORER && state1 == TREND_HEATMAP) {
                int c = -1, r = -1;
                for (int rr = 0; rr < 9; rr++) for (int cc = 0; cc < 18; cc++)
                    if (element_grid[rr][cc] == selected_element_idx) { c=cc; r=rr; break; }
                if (c >= 0) {
                    int nc = c + 1;
                    while (nc < 18 && element_grid[r][nc] == -1) nc++;
                    if (nc < 18) selected_element_idx = element_grid[r][nc];
                    else if (selected_element_idx < NUM_ELEMENTS - 1)
                        selected_element_idx++;
                }
            } else if (state0 == TREND_EXPLORER && state1 == TREND_SCATTER) {
                scatter_cursor_idx++;
            }

        // ── Up ────────────────────────────────────────────────────────────────
        } else if (key == KEY_UP) {
            if (state0 == POLYATOMIC_LIST) {
                if (poly_selected_idx > 0) { poly_selected_idx--; poly_name_scroll = 0; }
            } else if (state0 == PEROIODIC_TABLE && state1 == ELEMENT_LIST) {
                int c = -1, r = -1;
                for (int rr = 0; rr < 9; rr++) for (int cc = 0; cc < 18; cc++)
                    if (element_grid[rr][cc] == selected_element_idx) { c=cc; r=rr; break; }
                if (c >= 0) {
                    int nr = r - 1;
                    while (nr >= 0 && element_grid[nr][c] == -1) nr--;
                    if (nr >= 0) selected_element_idx = element_grid[nr][c];
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
                for (int rr = 0; rr < 9; rr++) for (int cc = 0; cc < 18; cc++)
                    if (element_grid[rr][cc] == selected_element_idx) { c=cc; r=rr; break; }
                if (c >= 0) {
                    int nr = r - 1;
                    while (nr >= 0 && element_grid[nr][c] == -1) nr--;
                    if (nr >= 0) selected_element_idx = element_grid[nr][c];
                }
            } else if (state0 == TREND_EXPLORER && state1 == TREND_SCATTER_OPTIONS) {
                if (scatter_x_trend_idx > 0) scatter_x_trend_idx--;
            }

        // ── Down ──────────────────────────────────────────────────────────────
        } else if (key == KEY_DOWN) {
            if (state0 == POLYATOMIC_LIST) {
                poly_selected_idx++; poly_name_scroll = 0;
            } else if (state0 == PEROIODIC_TABLE && state1 == ELEMENT_LIST) {
                int c = -1, r = -1;
                for (int rr = 0; rr < 9; rr++) for (int cc = 0; cc < 18; cc++)
                    if (element_grid[rr][cc] == selected_element_idx) { c=cc; r=rr; break; }
                if (c >= 0) {
                    int nr = r + 1;
                    while (nr < 9 && element_grid[nr][c] == -1) nr++;
                    if (nr < 9) selected_element_idx = element_grid[nr][c];
                }
            } else if (state0 == PEROIODIC_TABLE && state1 == ELEMENT_DETAIL) {
                if (detail_scroll_offset < 19 - 8) detail_scroll_offset++;
            } else if (state0 == PEROIODIC_TABLE && state1 == COLOR_OPTION_MENU) {
                if (selected_color_menu_idx < 1) selected_color_menu_idx++;
            } else if (state0 == TREND_EXPLORER && state1 == TREND_LIST) {
                if (selected_trend_idx < NUM_TRENDS - 1) selected_trend_idx++;
            } else if (state0 == TREND_EXPLORER && state1 == TREND_DETAIL) {
                if (detail_trend_menu_idx < 1) detail_trend_menu_idx++;
            } else if (state0 == TREND_EXPLORER && state1 == TREND_HEATMAP) {
                int c = -1, r = -1;
                for (int rr = 0; rr < 9; rr++) for (int cc = 0; cc < 18; cc++)
                    if (element_grid[rr][cc] == selected_element_idx) { c=cc; r=rr; break; }
                if (c >= 0) {
                    int nr = r + 1;
                    while (nr < 9 && element_grid[nr][c] == -1) nr++;
                    if (nr < 9) selected_element_idx = element_grid[nr][c];
                }
            } else if (state0 == TREND_EXPLORER && state1 == TREND_SCATTER_OPTIONS) {
                if (scatter_x_trend_idx < NUM_TRENDS - 1) scatter_x_trend_idx++;
            }

        // ── Polyatomic search text input ───────────────────────────────────────
        } else if (state0 == POLYATOMIC_LIST) {
            if (key >= KEY_A && key <= KEY_Z) {
                if (poly_search_len < 20) {
                    poly_search[poly_search_len++] = (char)('a' + (key - KEY_A));
                    poly_search[poly_search_len] = '\0';
                    poly_scroll_offset = 0; poly_selected_idx = 0;
                }
            } else if (key >= KEY_0 && key <= KEY_9) {
                if (poly_search_len < 20) {
                    poly_search[poly_search_len++] = (char)('0' + (key - KEY_0));
                    poly_search[poly_search_len] = '\0';
                    poly_scroll_offset = 0; poly_selected_idx = 0;
                }
            } else if (key == KEY_DEL) {
                if (poly_search_len > 0) {
                    poly_search[--poly_search_len] = '\0';
                    poly_scroll_offset = 0; poly_selected_idx = 0;
                }
            }
        }
    }
}
