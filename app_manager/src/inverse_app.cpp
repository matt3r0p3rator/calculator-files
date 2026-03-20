#include "../include/inverse_solver.hpp"
#include "../include/inverse_app.hpp"
#include "../include/ti_gui.hpp"
#include <vector>
#include <string>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <algorithm>

using namespace std;

enum InvAppState {
    STATE_DIMS,
    STATE_INPUT,
    STATE_SOLVING
};

// Populate grid with the n×n identity matrix as a placeholder
static void fill_identity(vector<vector<string>>& grid, int n) {
    grid.assign(n, vector<string>(n, "0"));
    for (int i = 0; i < n; i++) grid[i][i] = "1";
}

void InverseApp::run(TIGui& gui) {
    InvAppState state = STATE_DIMS;

    int matrix_size = 3;   // n×n

    vector<vector<string>> grid;
    int cursor_r = 0, cursor_c = 0;

    vector<InvStep> steps;
    int current_step = 0;

    bool running = true;
    while (running) {
        gui.clear(COLOR_WHITE);

        // ── Dimension selection ──────────────────────────────────────────────
        if (state == STATE_DIMS) {
            gui.drawTopBar("Matrix Size Setup");

            gui.drawText(20, 30, "Configure your matrix size:", COLOR_DARK_GRAY);

            gui.drawText(60, 80, "Size: ", COLOR_BLACK);
            gui.fillRect(120, 78, 40, 20, COLOR_BLUE);
            gui.drawText(135, 80, to_string(matrix_size), COLOR_WHITE);

            gui.drawText(30, 110, "(Square matrix: size x size)", COLOR_DARK_GRAY);

            gui.drawBottomBar(" </>: Change Size | ENTER: Create | ESC: Quit");
            gui.render();

            SDL_Event event;
            if (SDL_WaitEvent(&event)) {
                if (event.type == SDL_KEYDOWN) {
                    int sym = event.key.keysym.sym;
                    if (sym == SDLK_ESCAPE) {
                        running = false;
                    } else if (sym == SDLK_LEFT || sym == SDLK_KP4) {
                        if (matrix_size > 2) matrix_size--;
                    } else if (sym == SDLK_RIGHT || sym == SDLK_KP6) {
                        if (matrix_size < 6) matrix_size++;
                    } else if (sym == SDLK_RETURN || sym == SDLK_KP_ENTER) {
                        fill_identity(grid, matrix_size);
                        cursor_r = 0;
                        cursor_c = 0;
                        state = STATE_INPUT;
                    }
                }
            }

        // ── Matrix input ─────────────────────────────────────────────────────
        } else if (state == STATE_INPUT) {
            char title[50];
            sprintf(title, "Matrix Input (%dx%d)", matrix_size, matrix_size);
            gui.drawTopBar(title);
            gui.drawText(10, 25, "Use numbers, [-], [TAB] for next cell", COLOR_DARK_GRAY);

            // Auto-scale cells to fit screen
            int cell_w = std::min(70, 310 / matrix_size);
            int cell_h = std::min(25, 170 / matrix_size);

            int grid_start_x = (320 - (matrix_size * cell_w)) / 2;
            int grid_start_y = 50 + (160 - (matrix_size * cell_h)) / 2;

            gui.drawRect(grid_start_x - 5, grid_start_y - 5,
                         matrix_size * cell_w + 5, matrix_size * cell_h + 5, COLOR_LIGHT_GRAY);

            for (int r = 0; r < matrix_size; r++) {
                int y = grid_start_y + r * cell_h;
                for (int c = 0; c < matrix_size; c++) {
                    int x = grid_start_x + c * cell_w;

                    if (r == cursor_r && c == cursor_c) {
                        gui.fillRect(x - 2, y - 2, cell_w - 4, cell_h - 4, COLOR_BLUE);
                    } else {
                        gui.drawRect(x - 2, y - 2, cell_w - 4, cell_h - 4, COLOR_LIGHT_GRAY);
                    }

                    string display_str = grid[r][c];
                    if (display_str.empty()) display_str = "_";
                    if (display_str.length() > 6) display_str = display_str.substr(0, 5) + "..";

                    TIGuiColor text_color = (r == cursor_r && c == cursor_c) ? COLOR_WHITE : COLOR_BLACK;
                    gui.drawText(x, y, display_str, text_color);
                }
            }

            gui.drawBottomBar(" DPad: Move  |  TAB: Next  |  ENTER: Solve  |  ESC: Back");
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
                        if (cursor_r < matrix_size - 1) cursor_r++;
                    } else if (sym == SDLK_LEFT || sym == SDLK_KP4) {
                        if (cursor_c > 0) cursor_c--;
                    } else if (sym == SDLK_RIGHT || sym == SDLK_KP6) {
                        if (cursor_c < matrix_size - 1) cursor_c++;
                    } else if (sym == SDLK_TAB) {
                        if (cursor_c < matrix_size - 1) {
                            cursor_c++;
                        } else if (cursor_r < matrix_size - 1) {
                            cursor_c = 0;
                            cursor_r++;
                        }
                    } else if (sym == SDLK_RETURN || sym == SDLK_KP_ENTER) {
                        vector<vector<double>> init_mat(matrix_size, vector<double>(matrix_size, 0.0));
                        for (int r = 0; r < matrix_size; r++) {
                            for (int c = 0; c < matrix_size; c++) {
                                if (!grid[r][c].empty() && grid[r][c] != "-" && grid[r][c] != ".") {
                                    init_mat[r][c] = atof(grid[r][c].c_str());
                                }
                            }
                        }
                        steps = solve_matrix_inverse(init_mat);
                        current_step = 0;
                        state = STATE_SOLVING;
                    } else {
                        gui.handleTextInput(sym, grid[cursor_r][cursor_c]);
                    }
                }
            }

        // ── Step-by-step solving ─────────────────────────────────────────────
        } else if (state == STATE_SOLVING) {
            gui.drawTopBar("Matrix Inverse - Step by Step");

            const InvStep& s = steps[current_step];
            gui.drawText(10, 25, s.msg, COLOR_BLUE);

            // Display [A | I] rows as text, matching gauss style
            int row_spacing = std::min(20, 140 / matrix_size);
            int y = 48;

            for (int row = 0; row < matrix_size; row++) {
                string r_str = "[ ";
                for (int col = 0; col < matrix_size; col++) {
                    char buf[20];
                    sprintf(buf, "%5.2f  ", s.matrix[row][col]);
                    r_str += buf;
                }
                r_str += "| ";
                for (int col = 0; col < matrix_size; col++) {
                    char buf[20];
                    sprintf(buf, "%5.2f  ", s.inverse[row][col]);
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
                    int sym = event.key.keysym.sym;
                    if (sym == SDLK_RIGHT || sym == SDLK_DOWN ||
                        sym == SDLK_KP6  || sym == SDLK_KP2) {
                        if (current_step < (int)steps.size() - 1) current_step++;
                    } else if (sym == SDLK_LEFT || sym == SDLK_UP ||
                               sym == SDLK_KP4 || sym == SDLK_KP8) {
                        if (current_step > 0) current_step--;
                    } else if (sym == SDLK_TAB) {
                        if (current_step < (int)steps.size() - 1) current_step++;
                    } else if (sym == SDLK_ESCAPE) {
                        state = STATE_INPUT;
                    }
                }
            }
        }
    }
}