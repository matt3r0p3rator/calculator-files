#ifndef INVERSE_SOLVER_HPP
#define INVERSE_SOLVER_HPP

#define INV_MAX_N     6
#define INV_MAX_STEPS 80

struct InvStep {
    char msg[100];
    float matrix[INV_MAX_N][INV_MAX_N];   // float saves BSS on CE
    float inverse[INV_MAX_N][INV_MAX_N];
    int n;
};

// Returns number of steps written into out_steps.
int solve_matrix_inverse(double matrix[][INV_MAX_N], int n, InvStep* out_steps);

#endif // INVERSE_SOLVER_HPP
