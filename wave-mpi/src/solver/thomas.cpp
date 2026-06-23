#include "tridiagonal_solver.hpp"

#include <cmath>
#include <vector>

namespace {

constexpr double kPivotTolerance = 1e-14;

bool is_pivot_valid(double pivot) {
    return std::abs(pivot) > kPivotTolerance;
}

bool vectors_same_size(
    const std::vector<double>& lower,
    const std::vector<double>& diagonal,
    const std::vector<double>& upper,
    const std::vector<double>& rhs
) {
    const std::size_t n = lower.size();
    return n > 0 &&
           diagonal.size() == n &&
           upper.size() == n &&
           rhs.size() == n;
}

}  // namespace

bool solve_thomas(
    const std::vector<double>& lower,
    const std::vector<double>& diagonal,
    const std::vector<double>& upper,
    const std::vector<double>& rhs,
    std::vector<double>& solution
) {
    const int n = static_cast<int>(lower.size());

    if (n == 0 || !vectors_same_size(lower, diagonal, upper, rhs)) {
        return false;
    }

    std::vector<double> modified_diagonal = diagonal;
    std::vector<double> modified_rhs = rhs;

    if (n == 1) {
        if (!is_pivot_valid(modified_diagonal[0])) {
            return false;
        }
        solution.assign(1, modified_rhs[0] / modified_diagonal[0]);
        return true;
    }

    for (int i = 1; i < n; ++i) {
        if (!is_pivot_valid(modified_diagonal[i - 1])) {
            return false;
        }

        const double factor = lower[i] / modified_diagonal[i - 1];
        modified_diagonal[i] -= factor * upper[i - 1];
        modified_rhs[i] -= factor * modified_rhs[i - 1];

        if (!is_pivot_valid(modified_diagonal[i])) {
            return false;
        }
    }

    solution.assign(n, 0.0);
    solution[n - 1] = modified_rhs[n - 1] / modified_diagonal[n - 1];

    for (int i = n - 2; i >= 0; --i) {
        solution[i] =
            (modified_rhs[i] - upper[i] * solution[i + 1]) /
            modified_diagonal[i];
    }

    return true;
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
