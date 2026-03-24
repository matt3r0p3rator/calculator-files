#include "../include/inverse_solver.hpp"
#include <cstdio>
#include <cmath>
#include <cstring>

static void copy_sq(double dst[][INV_MAX_N], const double src[][INV_MAX_N], int n) {
    for (int r = 0; r < n; r++)
        for (int c = 0; c < n; c++)
            dst[r][c] = src[r][c];
}

static void copy_sq_f(float dst[][INV_MAX_N], const double src[][INV_MAX_N], int n) {
    for (int r = 0; r < n; r++)
        for (int c = 0; c < n; c++)
            dst[r][c] = (float)src[r][c];
}

static void swap_sq_rows(double a[][INV_MAX_N], double b[][INV_MAX_N],
                         int r1, int r2, int n) {
    for (int c = 0; c < n; c++) {
        double t1 = a[r1][c]; a[r1][c] = a[r2][c]; a[r2][c] = t1;
        double t2 = b[r1][c]; b[r1][c] = b[r2][c]; b[r2][c] = t2;
    }
}

static void snapshot_inv(InvStep* s, const char* msg,
                         double mat[][INV_MAX_N],
                         double inv[][INV_MAX_N], int n) {
    int len = (int)strlen(msg);
    if (len >= (int)sizeof(s->msg) - 1) len = (int)sizeof(s->msg) - 1;
    memcpy(s->msg, msg, len);
    s->msg[len] = '\0';
    copy_sq_f(s->matrix, mat, n);
    copy_sq_f(s->inverse, inv, n);
    s->n = n;
}

int solve_matrix_inverse(double in_matrix[][INV_MAX_N], int n, InvStep* out) {
    if (n == 0) return 0;

    double mat[INV_MAX_N][INV_MAX_N];
    double inv[INV_MAX_N][INV_MAX_N];
    copy_sq(mat, in_matrix, n);
    for (int r = 0; r < n; r++)
        for (int c = 0; c < n; c++)
            inv[r][c] = (r == c) ? 1.0 : 0.0;

    int steps = 0;
    char buf[100];

    snapshot_inv(&out[steps++], "Starting matrix and identity", mat, inv, n);

    for (int col = 0; col < n; col++) {
        bool found_pivot = false;
        for (int row = col; row < n; row++) {
            snprintf(buf, sizeof(buf), "Working on column %d", col + 1);
            snapshot_inv(&out[steps++], buf, mat, inv, n);

            if (fabs(mat[row][col]) > 1e-9) {
                if (row != col) {
                    swap_sq_rows(mat, inv, row, col, n);
                    snprintf(buf, sizeof(buf), "R%d <-> R%d", row + 1, col + 1);
                    snapshot_inv(&out[steps++], buf, mat, inv, n);
                }
                found_pivot = true;
                break;
            }
        }

        if (!found_pivot) {
            snapshot_inv(&out[steps++],
                         "Matrix is singular, no inverse exists",
                         mat, inv, n);
            return steps;
        }

        double pivot_val = mat[col][col];
        if (fabs(pivot_val - 1.0) > 1e-9) {
            for (int i = 0; i < n; i++) {
                mat[col][i] /= pivot_val;
                inv[col][i] /= pivot_val;
            }
            snprintf(buf, sizeof(buf), "R%d -> (1 / %.4f) * R%d",
                     col + 1, pivot_val, col + 1);
            snapshot_inv(&out[steps++], buf, mat, inv, n);
        }

        for (int r = 0; r < n; r++) {
            if (r == col) continue;
            double factor = mat[r][col];
            if (fabs(factor) > 1e-9) {
                for (int i = 0; i < n; i++) {
                    mat[r][i] -= factor * mat[col][i];
                    inv[r][i] -= factor * inv[col][i];
                }
                snprintf(buf, sizeof(buf), "R%d -> R%d - (%.4f) * R%d",
                         r + 1, r + 1, factor, col + 1);
                snapshot_inv(&out[steps++], buf, mat, inv, n);
            }
        }
    }

    snapshot_inv(&out[steps++], "Inversion complete", mat, inv, n);
    return steps;
}
