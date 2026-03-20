#include "../include/inverse_solver.hpp"
#include <vector>
#include <string>
#include <cstdio>
#include <cmath>
#include <algorithm>

using namespace std;

vector<InvStep> solve_matrix_inverse(vector<vector<double>> matrix) {
    vector<InvStep> steps;
    int n = matrix.size();
    if (n == 0) return steps;
    int m = matrix[0].size();
    if (n != m) {
        steps.push_back({"Error: Matrix must be square", matrix, {}});
        return steps;
    }
    
    vector<vector<double>> inverse(n, vector<double>(n, 0.0));
    for (int i = 0; i < n; i++) inverse[i][i] = 1.0; // Start with identity
    
    steps.push_back({"Starting matrix and identity", matrix, inverse});
    
    for (int col = 0; col < n; col++) {
        char buf[100];
        
        bool found_pivot = false;
        for (int row = col; row < n; row++) {
            sprintf(buf, "Working on column %d", col + 1);
            steps.push_back({string(buf), matrix, inverse});
            
            if (abs(matrix[row][col]) > 1e-9) {
                if (row != col) {
                    swap(matrix[row], matrix[col]);
                    swap(inverse[row], inverse[col]);
                    sprintf(buf, "R%d <-> R%d", row + 1, col + 1);
                    steps.push_back({string(buf), matrix, inverse});
                }
                found_pivot = true;
                break;
            }
        }
        
        if (!found_pivot) {
            steps.push_back({"Matrix is singular, no inverse exists", matrix, inverse});
            return steps;
        }
        
        double pivot_val = matrix[col][col];
        if (abs(pivot_val - 1.0) > 1e-9) {
            for (int i = 0; i < n; i++) {
                matrix[col][i] /= pivot_val;
                inverse[col][i] /= pivot_val;
            }
            sprintf(buf, "R%d -> (1 / %.4f) * R%d", col + 1, pivot_val, col + 1);
            steps.push_back({string(buf), matrix, inverse});
        }
        
        for (int r = 0; r < n; r++) {
            if (r == col) continue;
            double factor = matrix[r][col];
            if (abs(factor) > 1e-9) {
                for (int i = 0; i < n; i++) {
                    matrix[r][i] -= factor * matrix[col][i];
                    inverse[r][i] -= factor * inverse[col][i];
                }
                sprintf(buf, "R%d -> R%d - (%.4f) * R%d", r + 1, r + 1, factor, col + 1);
                steps.push_back({string(buf), matrix, inverse});
            }
        }
    }
    
    steps.push_back({"Inversion complete", matrix, inverse});
    return steps;
}