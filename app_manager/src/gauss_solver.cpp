#include "../include/gauss_solver.hpp"
#include <vector>
#include <string>
#include <cstdio>
#include <cmath>
#include <algorithm>

using namespace std;

vector<Step> solve_gauss_jordan(vector<vector<double>> matrix) {
    vector<Step> steps;
    int n_rows = matrix.size();
    if (n_rows == 0) return steps;
    int n_cols = matrix[0].size();
    
    steps.push_back({"Starting matrix", matrix});
    
    int col = 0;
    for (int row = 0; row < n_rows && col < n_cols; row++) {
        char buf[100];
        
        bool found_pivot = false;
        while (col < n_cols && !found_pivot) {
            sprintf(buf, "Working on column %d", col + 1);
            steps.push_back({string(buf), matrix});
            
            if (abs(matrix[row][col]) < 1e-9) {
                bool swapped = false;
                for (int swap_row = row + 1; swap_row < n_rows; swap_row++) {
                    if (abs(matrix[swap_row][col]) > 1e-9) {
                        swap(matrix[row], matrix[swap_row]);
                        sprintf(buf, "R%d <-> R%d", row + 1, swap_row + 1);
                        steps.push_back({string(buf), matrix});
                        swapped = true;
                        break;
                    }
                }
                if (!swapped) {
                    col++;
                    continue;
                }
            }
            found_pivot = true;
        }
        
        if (!found_pivot) break; // Finished all columns
        
        double pivot_val = matrix[row][col];
        if (abs(pivot_val - 1.0) > 1e-9) {
            for (int i = 0; i < n_cols; i++) matrix[row][i] /= pivot_val;
            sprintf(buf, "R%d  ->  (1 / %.4f) * R%d", row + 1, pivot_val, row + 1);
            steps.push_back({string(buf), matrix});
        }
        
        for (int r = 0; r < n_rows; r++) {
            if (r == row) continue;
            double factor = matrix[r][col];
            if (abs(factor) > 1e-9) {
                for (int i = 0; i < n_cols; i++) {
                    matrix[r][i] -= factor * matrix[row][i];
                }
                char sign = factor > 0 ? '-' : '+';
                sprintf(buf, "R%d  ->  R%d %c (%.4f * R%d)", r + 1, r + 1, sign, abs(factor), row + 1);
                steps.push_back({string(buf), matrix});
            }
        }
        col++;
    }
    
    steps.push_back({"RREF reached", matrix});
    return steps;
}