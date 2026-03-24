// ─────────────────────────────────────────────────────────────────────────────
// gauss_app.cpp – TI-84 CE port (STL-free)
// ─────────────────────────────────────────────────────────────────────────────
#include "../include/gauss_app.hpp"
#include "../include/ce_gui.hpp"
#include "../include/gauss_solver.hpp"
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <cstring>

enum AppState { STATE_DIMS, STATE_INPUT, STATE_SOLVING };

void GaussApp::run(TIGui& gui) {
    AppState state = STATE_DIMS;

    int num_rows = 3;
    int num_cols = 4;
    int dim_sel  = 0;   // 0 = rows, 1 = cols

    // Grid of text cells – each cell is a 14-char null-terminated string
    char grid[GAUSS_MAX_N][GAUSS_MAX_COLS][16];
    int cursor_r = 0, cursor_c = 0;

    static Step steps[GAUSS_MAX_STEPS];
    int n_steps      = 0;
    int current_step = 0;

    // Initialise grid to empty
    for (int r = 0; r < GAUSS_MAX_N; r++)
        for (int c = 0; c < GAUSS_MAX_COLS; c++)
            grid[r][c][0] = '\0';

    bool running = true;
    while (running) {
        gui.clear(COLOR_WHITE);

        // ── Dimension-selection screen ────────────────────────────────────────
        if (state == STATE_DIMS) {
            gui.drawTopBar("Matrix Dimensions Setup");
            gui.drawText(20, 30, "Configure your workspace:", COLOR_DARK_GRAY);

            char rs[4], cs[4];
            snprintf(rs, sizeof(rs), "%d", num_rows);
            snprintf(cs, sizeof(cs), "%d", num_cols);

            gui.drawText(60, 80, "Rows: ", COLOR_BLACK);
            gui.fillRect(120, 78, 40, 20, dim_sel == 0 ? COLOR_BLUE : COLOR_LIGHT_GRAY);
            gui.drawText(135, 80, rs, dim_sel == 0 ? COLOR_WHITE : COLOR_BLACK);

            gui.drawText(60, 120, "Cols: ", COLOR_BLACK);
            gui.fillRect(120, 118, 40, 20, dim_sel == 1 ? COLOR_BLUE : COLOR_LIGHT_GRAY);
            gui.drawText(135, 120, cs, dim_sel == 1 ? COLOR_WHITE : COLOR_BLACK);

            gui.drawBottomBar(" ^/v:Select | </> :Change | ENTER:Create | CLR:Quit");
            gui.render();

            int key = gui.waitKey();
            if (key == KEY_ESCAPE) {
                running = false;
            } else if (key == KEY_UP) {
                dim_sel = 0;
            } else if (key == KEY_DOWN) {
                dim_sel = 1;
            } else if (key == KEY_LEFT) {
                if (dim_sel == 0 && num_rows > 1) num_rows--;
                if (dim_sel == 1 && num_cols > 1) num_cols--;
            } else if (key == KEY_RIGHT) {
                if (dim_sel == 0 && num_rows < GAUSS_MAX_N)         num_rows++;
                if (dim_sel == 1 && num_cols < GAUSS_MAX_COLS)      num_cols++;
            } else if (key == KEY_ENTER) {
                // Clear grid for the chosen dimensions
                for (int r = 0; r < num_rows; r++)
                    for (int c = 0; c < num_cols; c++)
                        grid[r][c][0] = '\0';
                cursor_r = cursor_c = 0;
                state = STATE_INPUT;
            }

        // ── Matrix data-entry screen ─────────────────────────────────────────
        } else if (state == STATE_INPUT) {
            char title[50];
            snprintf(title, sizeof(title), "Matrix Input (%dx%d)", num_rows, num_cols);
            gui.drawTopBar(title);
            gui.drawText(10, 25, "Numbers/(-)/. | 2nd=Next | ENTER=Solve",
                         COLOR_DARK_GRAY);

            int cell_w = 310 / num_cols;
            if (cell_w > 70) cell_w = 70;
            int cell_h = 170 / num_rows;
            if (cell_h > 25) cell_h = 25;
            int grid_start_x = (320 - num_cols * cell_w) / 2;
            int grid_start_y = 50 + (160 - num_rows * cell_h) / 2;

            gui.drawRect(grid_start_x - 5, grid_start_y - 5,
                         num_cols * cell_w + 5, num_rows * cell_h + 5,
                         COLOR_LIGHT_GRAY);

            for (int r = 0; r < num_rows; r++) {
                int y = grid_start_y + r * cell_h;
                for (int c = 0; c < num_cols; c++) {
                    int x = grid_start_x + c * cell_w;
                    if (r == cursor_r && c == cursor_c) {
                        gui.fillRect(x - 2, y - 2, cell_w - 4, cell_h - 4, COLOR_BLUE);
                    } else {
                        gui.drawRect(x - 2, y - 2, cell_w - 4, cell_h - 4, COLOR_LIGHT_GRAY);
                    }
                    char disp[16];
                    if (grid[r][c][0] == '\0') {
                        disp[0] = '_'; disp[1] = '\0';
                    } else if ((int)strlen(grid[r][c]) > 6) {
                        snprintf(disp, sizeof(disp), "%.5s..", grid[r][c]);
                    } else {
                        snprintf(disp, sizeof(disp), "%s", grid[r][c]);
                    }
                    TIGuiColor tc = (r == cursor_r && c == cursor_c)
                        ? COLOR_WHITE : COLOR_BLACK;
                    gui.drawText(x, y, disp, tc);
                }
            }

            gui.drawBottomBar(" DPad:Move | 2nd:Next | ENTER:Solve | CLR:Back");
            gui.render();

            int key = gui.waitKey();
            if (key == KEY_ESCAPE) {
                state = STATE_DIMS;
            } else if (key == KEY_UP    && cursor_r > 0) {
                cursor_r--;
            } else if (key == KEY_DOWN  && cursor_r < num_rows - 1) {
                cursor_r++;
            } else if (key == KEY_LEFT  && cursor_c > 0) {
                cursor_c--;
            } else if (key == KEY_RIGHT && cursor_c < num_cols - 1) {
                cursor_c++;
            } else if (key == KEY_TAB) {
                if (cursor_c < num_cols - 1) {
                    cursor_c++;
                } else if (cursor_r < num_rows - 1) {
                    cursor_c = 0; cursor_r++;
                }
            } else if (key == KEY_ENTER) {
                double init_mat[GAUSS_MAX_N][GAUSS_MAX_COLS];
                for (int r = 0; r < num_rows; r++)
                    for (int c = 0; c < num_cols; c++) {
                        const char* cell = grid[r][c];
                        init_mat[r][c] =
                            (cell[0] != '\0' &&
                             !(cell[0] == '-' && cell[1] == '\0') &&
                             !(cell[0] == '.' && cell[1] == '\0'))
                            ? atof(cell) : 0.0;
                    }
                n_steps = solve_gauss_jordan(init_mat, num_rows, num_cols, steps);
                current_step = 0;
                state = STATE_SOLVING;
            } else {
                gui.handleTextInput(key, grid[cursor_r][cursor_c], 14);
            }

        // ── Step-by-step iteration viewer ────────────────────────────────────
        } else if (state == STATE_SOLVING) {
            gui.drawTopBar("Gauss-Jordan Iteration");

            const Step& s = steps[current_step];
            gui.drawText(10, 25, s.msg, COLOR_BLUE);

            int row_spacing = 160 / s.rows;
            if (row_spacing > 20) row_spacing = 20;
            int y = 50;
            for (int row = 0; row < s.rows; row++) {
                char r_str[120];
                int pos = 0;
                pos += snprintf(r_str + pos, sizeof(r_str) - pos, "[ ");
                for (int col = 0; col < s.cols; col++) {
                    if (col == s.cols - 1 && s.cols > s.rows)
                        pos += snprintf(r_str + pos, sizeof(r_str) - pos,
                                        "| %5.2f ", s.matrix[row][col]);
                    else
                        pos += snprintf(r_str + pos, sizeof(r_str) - pos,
                                        "%5.2f  ",  s.matrix[row][col]);
                }
                snprintf(r_str + pos, sizeof(r_str) - pos, "]");
                gui.drawText(10, y, r_str, COLOR_BLACK);
                y += row_spacing;
            }

            char prog[50];
            snprintf(prog, sizeof(prog), "Step %d of %d",
                     current_step + 1, n_steps);
            gui.drawText(10, y + 10, prog, COLOR_DARK_GRAY);

            gui.drawBottomBar(" </> or 2nd: Step  |  CLR: Back to Grid");
            gui.render();

            int key = gui.waitKey();
            if (key == KEY_RIGHT || key == KEY_DOWN || key == KEY_TAB) {
                if (current_step < n_steps - 1) current_step++;
            } else if (key == KEY_LEFT || key == KEY_UP) {
                if (current_step > 0) current_step--;
            } else if (key == KEY_ESCAPE) {
                state = STATE_INPUT;
            }
        }
    }
}

