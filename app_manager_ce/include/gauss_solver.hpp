#ifndef SOLVER_HPP
#define SOLVER_HPP

#define GAUSS_MAX_N     4
#define GAUSS_MAX_COLS  5   // 4 unknowns + augmented column
#define GAUSS_MAX_STEPS 50

struct Step {
    char msg[100];
    float matrix[GAUSS_MAX_N][GAUSS_MAX_COLS];  // float saves BSS on CE
    int rows, cols;
};

// Returns number of steps written into out_steps.
int solve_gauss_jordan(double matrix[][GAUSS_MAX_COLS], int rows, int cols,
                       Step* out_steps);

#endif // SOLVER_HPP
