#ifndef INVERSE_SOLVER_HPP
#define INVERSE_SOLVER_HPP

#include <vector>
#include <string>

struct InvStep {
    std::string msg;
    std::vector<std::vector<double>> matrix;
    std::vector<std::vector<double>> inverse;
};

std::vector<InvStep> solve_matrix_inverse(std::vector<std::vector<double>> matrix);

#endif // INVERSE_SOLVER_HPP