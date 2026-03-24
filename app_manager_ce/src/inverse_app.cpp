// ─────────────────────────────────────────────────────────────────────────────
// inverse_app.cpp – TI-84 CE port (STL-free)
// ─────────────────────────────────────────────────────────────────────────────
#include "../include/inverse_solver.hpp"
#include "../include/inverse_app.hpp"
#include "../include/ce_gui.hpp"
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <cstring>

enum InvAppState { STATE_DIMS, STATE_INPUT, STATE_SOLVING };

static void fill_identity(char grid[][INV_MAX_N][16], int n) {
    for (int r = 0; r < n; r++)
        for (int c = 0; c < n; c++) {
            grid[r][c][0] = (r == c) ? '1' : '0';
            grid[r][c][1] = '\0';
        }
}

void InverseApp::run(TIGui& gui) {
    InvAppState state = STATE_DIMS;
    int matrix_size = 3;

    char grid[INV_MAX_N][INV_MAX_N][16];
    int cursor_r = 0, cursor_c = 0;

    static InvStep steps[INV_MAX_STEPS];
    int n_steps = 0, current_step = 0;

    bool running = true;
    while (running) {
        gui.clear(COLOR_WHITE);

        // ── Size-selection screen ─────────────────────────────────────────────
        if (state == STATE_DIMS) {
            gui.drawTopBar("Matrix Size Setup");
            gui.drawText(20, 30, "Configure your matrix size:", COLOR_DARK_GRAY);

            char szbuf[4];
            snprintf(szbuf, sizeof(szbuf), "%d", matrix_size);

            gui.drawText(60, 80, "Size: ", COLOR_BLACK);
            gui.fillRect(120, 78, 40, 20, COLOR_BLUE);
            gui.drawText(135, 80, szbuf, COLOR_WHITE);
            gui.drawText(30, 110, "(Square matrix: size x size)", COLOR_DARK_GRAY);

            gui.drawBottomBar(" </> :Change Size | ENTER:Create | CLR:Quit");
            gui.render();

            int key = gui.waitKey();
            if (key == KEY_ESCAPE)                         running = false;
            else if (key == KEY_LEFT  && matrix_size > 2)  matrix_size--;
            else if (key == KEY_RIGHT && matrix_size < INV_MAX_N) matrix_size++;
            else if (key == KEY_ENTER) {
                fill_identity(grid, matrix_size);
                cursor_r = cursor_c = 0;
                state = STATE_INPUT;
            }

        // ── Matrix data-entry screen ─────────────────────────────────────────
        } else if (state == STATE_INPUT) {
            char title[50];
            snprintf(title, sizeof(title), "Matrix Input (%dx%d)",
                     matrix_size, matrix_size);
            gui.drawTopBar(title);
            gui.drawText(10, 25, "Numbers/(-)/. | 2nd=Next | ENTER=Solve",
                         COLOR_DARK_GRAY);

            int cell_w = 310 / matrix_size;
            if (cell_w > 70) cell_w = 70;
            int cell_h = 170 / matrix_size;
            if (cell_h > 25) cell_h = 25;
            int grid_start_x = (320 - matrix_size * cell_w) / 2;
            int grid_start_y = 50 + (160 - matrix_size * cell_h) / 2;

            gui.drawRect(grid_start_x - 5, grid_start_y - 5,
                         matrix_size * cell_w + 5, matrix_size * cell_h + 5,
                         COLOR_LIGHT_GRAY);

            for (int r = 0; r < matrix_size; r++) {
                int y = grid_start_y + r * cell_h;
                for (int c = 0; c < matrix_size; c++) {
                    int x = grid_start_x + c * cell_w;
                    if (r == cursor_r && c == cursor_c)
                        gui.fillRect(x - 2, y - 2, cell_w - 4, cell_h - 4, COLOR_BLUE);
                    else
                        gui.drawRect(x - 2, y - 2, cell_w - 4, cell_h - 4, COLOR_LIGHT_GRAY);

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
            } else if (key == KEY_DOWN  && cursor_r < matrix_size - 1) {
                cursor_r++;
            } else if (key == KEY_LEFT  && cursor_c > 0) {
                cursor_c--;
            } else if (key == KEY_RIGHT && cursor_c < matrix_size - 1) {
                cursor_c++;
            } else if (key == KEY_TAB) {
                if (cursor_c < matrix_size - 1) {
                    cursor_c++;
                } else if (cursor_r < matrix_size - 1) {
                    cursor_c = 0; cursor_r++;
                }
            } else if (key == KEY_ENTER) {
                double init_mat[INV_MAX_N][INV_MAX_N];
                for (int r = 0; r < matrix_size; r++)
                    for (int c = 0; c < matrix_size; c++) {
                        const char* cell = grid[r][c];
                        init_mat[r][c] =
                            (cell[0] != '\0' &&
                             !(cell[0] == '-' && cell[1] == '\0') &&
                             !(cell[0] == '.' && cell[1] == '\0'))
                            ? atof(cell) : 0.0;
                    }
                n_steps = solve_matrix_inverse(init_mat, matrix_size, steps);
                current_step = 0;
                state = STATE_SOLVING;
            } else {
                gui.handleTextInput(key, grid[cursor_r][cursor_c], 14);
            }

        // ── Step-by-step viewer ───────────────────────────────────────────────
        } else if (state == STATE_SOLVING) {
            gui.drawTopBar("Matrix Inverse - Step by Step");

            const InvStep& s = steps[current_step];
            gui.drawText(10, 25, s.msg, COLOR_BLUE);

            int row_spacing = 140 / matrix_size;
            if (row_spacing > 20) row_spacing = 20;
            int y = 48;
            for (int row = 0; row < matrix_size; row++) {
                char r_str[160];
                int pos = 0;
                pos += snprintf(r_str + pos, sizeof(r_str) - pos, "[ ");
                for (int col = 0; col < matrix_size; col++)
                    pos += snprintf(r_str + pos, sizeof(r_str) - pos,
                                    "%5.2f  ", s.matrix[row][col]);
                pos += snprintf(r_str + pos, sizeof(r_str) - pos, "| ");
                for (int col = 0; col < matrix_size; col++)
                    pos += snprintf(r_str + pos, sizeof(r_str) - pos,
                                    "%5.2f  ", s.inverse[row][col]);
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

