#pragma once

#include <string>
#include <vector>

bool solve_thomas(
    const std::vector<double>& lower,
    const std::vector<double>& diagonal,
    const std::vector<double>& upper,
    const std::vector<double>& rhs,
    std::vector<double>& solution
);

bool solve_cyclic_reduction(
    const std::vector<double>& lower,
    const std::vector<double>& diagonal,
    const std::vector<double>& upper,
    const std::vector<double>& rhs,
    std::vector<double>& solution
);

bool solve_tridiagonal(
    const std::string& solver_name,
    const std::vector<double>& lower,
    const std::vector<double>& diagonal,
    const std::vector<double>& upper,
    const std::vector<double>& rhs,
    std::vector<double>& solution
);
