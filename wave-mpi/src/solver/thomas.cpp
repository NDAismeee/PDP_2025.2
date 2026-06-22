#include "tridiagonal_solver.hpp"

bool solve_thomas(
    const std::vector<double>& lower,
    const std::vector<double>& diagonal,
    const std::vector<double>& upper,
    const std::vector<double>& rhs,
    std::vector<double>& solution
) {
    (void)lower;
    (void)diagonal;
    (void)upper;
    (void)rhs;
    (void)solution;
    return false;
}

bool solve_tridiagonal(
    const std::string& solver_name,
    const std::vector<double>& lower,
    const std::vector<double>& diagonal,
    const std::vector<double>& upper,
    const std::vector<double>& rhs,
    std::vector<double>& solution
) {
    if (solver_name == "thomas") {
        return solve_thomas(lower, diagonal, upper, rhs, solution);
    }

    if (solver_name == "cr") {
        return solve_cyclic_reduction(lower, diagonal, upper, rhs, solution);
    }

    return false;
}
