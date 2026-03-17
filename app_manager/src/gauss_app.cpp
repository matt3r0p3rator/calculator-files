#include "../include/gauss_app.hpp"
#include "../include/ti_gui.hpp"
#include "../include/gauss_solver.hpp"
#include <vector>
#include <string>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <algorithm>

using namespace std;

enum AppState {
    STATE_DIMS,
    STATE_INPUT,
    STATE_SOLVING
};

void GaussApp::run(TIGui& gui) {
    AppState state = STATE_DIMS;
    
    int num_rows = 3;
    int num_cols = 4;
    int dim_sel = 0; // 0 for rows, 1 for cols
    
    vector<vector<string>> grid;
    int cursor_r = 0, cursor_c = 0;
    
    vector<Step> steps;
    int current_step = 0;
    
    bool running = true;
    while(running) {
        gui.clear(COLOR_WHITE);
        
        if (state == STATE_DIMS) {
            gui.drawTopBar("Matrix Dimensions Setup");
            
            gui.drawText(20, 30, "Configure your workspace:", COLOR_DARK_GRAY);
            
            // Rows input
            gui.drawText(60, 80, "Rows: ", COLOR_BLACK);
            gui.fillRect(120, 78, 40, 20, dim_sel == 0 ? COLOR_BLUE : COLOR_LIGHT_GRAY);
            gui.drawText(135, 80, to_string(num_rows), dim_sel == 0 ? COLOR_WHITE : COLOR_BLACK);
            
            // Cols input
            gui.drawText(60, 120, "Cols: ", COLOR_BLACK);
            gui.fillRect(120, 118, 40, 20, dim_sel == 1 ? COLOR_BLUE : COLOR_LIGHT_GRAY);
            gui.drawText(135, 120, to_string(num_cols), dim_sel == 1 ? COLOR_WHITE : COLOR_BLACK);
            
            gui.drawBottomBar(" ^/v: Select | </>: Change | ENTER: Create | ESC: Quit");
            gui.render();
            
            SDL_Event event;
            if (SDL_WaitEvent(&event)) {
                if (event.type == SDL_KEYDOWN) {
                    int sym = event.key.keysym.sym;
                    if (sym == SDLK_ESCAPE) {
                        running = false;
                    } else if (sym == SDLK_UP || sym == SDLK_KP8) {
                        dim_sel = 0;
                    } else if (sym == SDLK_DOWN || sym == SDLK_KP2) {
                        dim_sel = 1;
                    } else if (sym == SDLK_LEFT || sym == SDLK_KP4) {
                        if (dim_sel == 0 && num_rows > 1) num_rows--;
                        if (dim_sel == 1 && num_cols > 1) num_cols--;
                    } else if (sym == SDLK_RIGHT || sym == SDLK_KP6) {
                        if (dim_sel == 0 && num_rows < 6) num_rows++;  // Limit to fit screen
                        if (dim_sel == 1 && num_cols < 6) num_cols++;
                    } else if (sym == SDLK_RETURN || sym == SDLK_KP_ENTER) {
                        grid.assign(num_rows, vector<string>(num_cols, ""));
                        cursor_r = 0;
                        cursor_c = 0;
                        state = STATE_INPUT;
                    }
                }
            }
        } else if (state == STATE_INPUT) {
            char title[50];
            sprintf(title, "Matrix Input (%dx%d)", num_rows, num_cols);
            gui.drawTopBar(title);
            gui.drawText(10, 25, "Use numbers, [-], [TAB] for next cell", COLOR_DARK_GRAY);
            
            // Calculte sizes to fit on screen
            int cell_w = std::min(70, 310 / num_cols);
            int cell_h = std::min(25, 170 / num_rows);
            
            int grid_start_x = (320 - (num_cols * cell_w)) / 2;
            int grid_start_y = 50 + (160 - (num_rows * cell_h)) / 2;
            
            gui.drawRect(grid_start_x - 5, grid_start_y - 5, num_cols * cell_w + 5, num_rows * cell_h + 5, COLOR_LIGHT_GRAY);
            
            for (int r = 0; r < num_rows; r++) {
                int y = grid_start_y + r * cell_h;
                for (int c = 0; c < num_cols; c++) {
                    int x = grid_start_x + c * cell_w;
                    
                    if (r == cursor_r && c == cursor_c) {
                        gui.fillRect(x - 2, y - 2, cell_w - 4, cell_h - 4, COLOR_BLUE);
                    } else {
                        gui.drawRect(x - 2, y - 2, cell_w - 4, cell_h - 4, COLOR_LIGHT_GRAY);
                    }
                    
                    string display_str = grid[r][c];
                    if (display_str.empty()) display_str = "_";
                    
                    // Simple display truncation
                    if (display_str.length() > 6) { 
                        display_str = display_str.substr(0, 5) + "..";
                    }
                    
                    TIGuiColor text_color = (r == cursor_r && c == cursor_c) ? COLOR_WHITE : COLOR_BLACK;
                    gui.drawText(x, y, display_str, text_color);
                }
            }
            
            gui.drawBottomBar(" DPad: Move  |  ENTER: Solve  |  ESC: Back");
            gui.render();
            
            SDL_Event event;
            if (SDL_WaitEvent(&event)) {
                if (event.type == SDL_KEYDOWN) {
                    int sym = event.key.keysym.sym;
                    if (sym == SDLK_ESCAPE) {
                        state = STATE_DIMS;
                    } else if (sym == SDLK_UP || sym == SDLK_KP8) {
                        if (cursor_r > 0) cursor_r--;
                    } else if (sym == SDLK_DOWN || sym == SDLK_KP2) {
                        if (cursor_r < num_rows - 1) cursor_r++;
                    } else if (sym == SDLK_LEFT || sym == SDLK_KP4) {
                        if (cursor_c > 0) cursor_c--;
                    } else if (sym == SDLK_RIGHT || sym == SDLK_KP6) {
                        if (cursor_c < num_cols - 1) cursor_c++;
                    } else if (sym = SDLK_TAB) {
                        if (cursor_c < num_cols - 1) {
                            cursor_c++;
                        } else if (cursor_r < num_rows - 1) {
                            cursor_c = 0;
                            cursor_r++;
                        }
                    }
                     else if (sym == SDLK_RETURN || sym == SDLK_KP_ENTER) {
                        vector<vector<double>> init_mat(num_rows, vector<double>(num_cols, 0.0));
                        for (int r = 0; r < num_rows; r++) {
                            for (int c = 0; c < num_cols; c++) {
                                if (!grid[r][c].empty() && grid[r][c] != "-" && grid[r][c] != ".") {
                                    init_mat[r][c] = atof(grid[r][c].c_str());
                                }
                            }
                        }
                        steps = solve_gauss_jordan(init_mat);
                        current_step = 0;
                        state = STATE_SOLVING;
                    } else {
                        gui.handleTextInput(sym, grid[cursor_r][cursor_c]);
                    }
                }
            }
        } else if (state == STATE_SOLVING) {
            gui.drawTopBar("Gauss-Jordan Iteration");
            
            const Step& s = steps[current_step];
            gui.drawText(10, 25, s.msg, COLOR_BLUE);
            
            // Auto scale rows
            int row_spacing = std::min(20, 160 / num_rows);
            int y = 50;
            
            for (size_t row = 0; row < s.matrix.size(); row++) {
                string r_str = "[ ";
                for (size_t col = 0; col < s.matrix[row].size(); col++) {
                    char buf[20];
                    if (col == s.matrix[row].size() - 1 && num_cols > num_rows) {
                        sprintf(buf, "| %5.2f ", s.matrix[row][col]);
                    } else {
                        sprintf(buf, "%5.2f  ", s.matrix[row][col]);
                    }
                    r_str += buf;
                }
                r_str += "]";
                gui.drawText(10, y, r_str, COLOR_BLACK);
                y += row_spacing;
            }
            
            char prog[50];
            sprintf(prog, "Step %d of %d", current_step + 1, (int)steps.size());
            gui.drawText(10, y + 10, string(prog), COLOR_DARK_GRAY);
            
            gui.drawBottomBar(" LEFT/RIGHT/TAB: Step  |  ESC: Back to Grid");
            gui.render();
            
            SDL_Event event;
            if (SDL_WaitEvent(&event)) {
                if (event.type == SDL_KEYDOWN) {
                    if (event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_DOWN || 
                        event.key.keysym.sym == SDLK_KP6 || event.key.keysym.sym == SDLK_KP2) {
                        if (current_step < (int)steps.size() - 1) current_step++;
                    } else if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_UP || 
                               event.key.keysym.sym == SDLK_KP4 || event.key.keysym.sym == SDLK_KP8) {
                        if (current_step > 0) current_step--;
                    } else if (event.key.keysym.sym == SDLK_TAB) {
                        if (current_step < (int)steps.size() - 1) current_step++;
                    }
                     else if (event.key.keysym.sym == SDLK_ESCAPE) {
                        state = STATE_INPUT;
                    }
                }
            }
        }
    }
}
