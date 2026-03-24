#include "../include/gauss_solver.hpp"
#include <cstdio>
#include <cmath>
#include <cstring>

static void copy_matrix(float dst[][GAUSS_MAX_COLS],
                        double src[][GAUSS_MAX_COLS],
                        int rows, int cols) {
    for (int r = 0; r < rows; r++)
        for (int c = 0; c < cols; c++)
            dst[r][c] = (float)src[r][c];
}

static void copy_dbl(double dst[][GAUSS_MAX_COLS],
                     double src[][GAUSS_MAX_COLS],
                     int rows, int cols) {
    for (int r = 0; r < rows; r++)
        for (int c = 0; c < cols; c++)
            dst[r][c] = src[r][c];
}

static void swap_rows(double mat[][GAUSS_MAX_COLS], int a, int b, int cols) {
    for (int c = 0; c < cols; c++) {
        double tmp  = mat[a][c];
        mat[a][c]   = mat[b][c];
        mat[b][c]   = tmp;
    }
}

static void snapshot(Step* s, const char* msg,
                     double mat[][GAUSS_MAX_COLS], int rows, int cols) {
    int n = (int)strlen(msg);
    if (n >= (int)sizeof(s->msg) - 1) n = (int)sizeof(s->msg) - 1;
    memcpy(s->msg, msg, n);
    s->msg[n] = '\0';
    copy_matrix(s->matrix, mat, rows, cols);
    s->rows = rows;
    s->cols = cols;
}

int solve_gauss_jordan(double in_matrix[][GAUSS_MAX_COLS],
                       int rows, int cols,
                       Step* out) {
    if (rows == 0 || cols == 0) return 0;

    double mat[GAUSS_MAX_N][GAUSS_MAX_COLS];
    copy_dbl(mat, in_matrix, rows, cols);

    int n = 0;
    char buf[100];

    snapshot(&out[n++], "Starting matrix", mat, rows, cols);

    int col = 0;
    for (int row = 0; row < rows && col < cols; row++) {
        bool found_pivot = false;
        while (col < cols && !found_pivot) {
            snprintf(buf, sizeof(buf), "Working on column %d", col + 1);
            snapshot(&out[n++], buf, mat, rows, cols);

            if (fabs(mat[row][col]) < 1e-9) {
                bool swapped = false;
                for (int sr = row + 1; sr < rows; sr++) {
                    if (fabs(mat[sr][col]) > 1e-9) {
                        swap_rows(mat, row, sr, cols);
                        snprintf(buf, sizeof(buf), "R%d <-> R%d", row + 1, sr + 1);
                        snapshot(&out[n++], buf, mat, rows, cols);
                        swapped = true;
                        break;
                    }
                }
                if (!swapped) { col++; continue; }
            }
            found_pivot = true;
        }

        if (!found_pivot) break;

        double pivot_val = mat[row][col];
        if (fabs(pivot_val - 1.0) > 1e-9) {
            for (int i = 0; i < cols; i++) mat[row][i] /= pivot_val;
            snprintf(buf, sizeof(buf), "R%d  ->  (1 / %.4f) * R%d",
                     row + 1, pivot_val, row + 1);
            snapshot(&out[n++], buf, mat, rows, cols);
        }

        for (int r = 0; r < rows; r++) {
            if (r == row) continue;
            double factor = mat[r][col];
            if (fabs(factor) > 1e-9) {
                for (int i = 0; i < cols; i++)
                    mat[r][i] -= factor * mat[row][i];
                char sign = factor > 0 ? '-' : '+';
                snprintf(buf, sizeof(buf), "R%d  ->  R%d %c (%.4f * R%d)",
                         r + 1, r + 1, sign, fabs(factor), row + 1);
                snapshot(&out[n++], buf, mat, rows, cols);
            }
        }
        col++;
    }

    snapshot(&out[n++], "RREF reached", mat, rows, cols);
    return n;
}
