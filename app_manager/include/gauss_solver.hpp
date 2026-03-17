#ifndef SOLVER_HPP
#define SOLVER_HPP

#include <vector>
#include <string>

struct Step {
    std::string msg;
    std::vector<std::vector<double>> matrix;
};

std::vector<Step> solve_gauss_jordan(std::vector<std::vector<double>> matrix);

#endif // SOLVER_HPP