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

bool solve_cyclic_reduction_impl(
    const std::vector<double>& lower,
    const std::vector<double>& diagonal,
    const std::vector<double>& upper,
    const std::vector<double>& rhs,
    std::vector<double>& solution
) {
    const int n = static_cast<int>(lower.size());

    if (n == 1) {
        if (!is_pivot_valid(diagonal[0])) {
            return false;
        }
        solution.assign(1, rhs[0] / diagonal[0]);
        return true;
    }

    if (n == 2) {
        return solve_thomas(lower, diagonal, upper, rhs, solution);
    }

    const int reduced_size = (n + 1) / 2;
    std::vector<double> new_lower(reduced_size, 0.0);
    std::vector<double> new_diagonal(reduced_size, 0.0);
    std::vector<double> new_upper(reduced_size, 0.0);
    std::vector<double> new_rhs(reduced_size, 0.0);

    int reduced_index = 0;
    for (int i = 0; i < n; i += 2) {
        double alpha = 0.0;
        double gamma = 0.0;

        if (i - 1 >= 0) {
            if (!is_pivot_valid(diagonal[i - 1])) {
                return false;
            }
            alpha = -lower[i] / diagonal[i - 1];
        }

        if (i + 1 < n) {
            if (!is_pivot_valid(diagonal[i + 1])) {
                return false;
            }
            gamma = -upper[i] / diagonal[i + 1];
        }

        double new_b = diagonal[i];
        double new_d = rhs[i];

        if (i - 1 >= 0) {
            new_b += alpha * upper[i - 1];
            new_d += alpha * rhs[i - 1];
        }

        if (i + 1 < n) {
            new_b += gamma * lower[i + 1];
            new_d += gamma * rhs[i + 1];
        }

        new_diagonal[reduced_index] = new_b;
        new_rhs[reduced_index] = new_d;
        new_lower[reduced_index] =
            (i - 2 >= 0) ? alpha * lower[i - 1] : 0.0;
        new_upper[reduced_index] =
            (i + 2 < n) ? gamma * upper[i + 1] : 0.0;

        ++reduced_index;
    }

    std::vector<double> reduced_solution;
    if (!solve_cyclic_reduction_impl(
            new_lower,
            new_diagonal,
            new_upper,
            new_rhs,
            reduced_solution)) {
        return false;
    }

    solution.assign(n, 0.0);
    for (int k = 0; k < reduced_size; ++k) {
        solution[2 * k] = reduced_solution[k];
    }

    for (int i = 1; i < n; i += 2) {
        if (!is_pivot_valid(diagonal[i])) {
            return false;
        }

        const double left_value =
            (i - 1 >= 0) ? solution[i - 1] : 0.0;
        const double right_value =
            (i + 1 < n) ? solution[i + 1] : 0.0;

        solution[i] =
            (rhs[i] - lower[i] * left_value - upper[i] * right_value) /
            diagonal[i];
    }

    return true;
}

}  // namespace

bool solve_cyclic_reduction(
    const std::vector<double>& lower,
    const std::vector<double>& diagonal,
    const std::vector<double>& upper,
    const std::vector<double>& rhs,
    std::vector<double>& solution
) {
    if (lower.empty() || !vectors_same_size(lower, diagonal, upper, rhs)) {
        return false;
    }

    return solve_cyclic_reduction_impl(lower, diagonal, upper, rhs, solution);
}
