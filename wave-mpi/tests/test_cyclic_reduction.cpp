#include <cmath>
#include <iostream>
#include <vector>

#include "tridiagonal_solver.hpp"

namespace {

constexpr double kTolerance = 1e-9;

bool nearly_equal(double a, double b) {
    return std::abs(a - b) <= kTolerance;
}

bool vectors_nearly_equal(
    const std::vector<double>& actual,
    const std::vector<double>& expected
) {
    if (actual.size() != expected.size()) {
        return false;
    }

    for (std::size_t i = 0; i < actual.size(); ++i) {
        if (!nearly_equal(actual[i], expected[i])) {
            return false;
        }
    }

    return true;
}

std::vector<double> compute_rhs(
    const std::vector<double>& lower,
    const std::vector<double>& diagonal,
    const std::vector<double>& upper,
    const std::vector<double>& expected
) {
    const int n = static_cast<int>(expected.size());
    std::vector<double> rhs(n, 0.0);

    for (int i = 0; i < n; ++i) {
        const double left =
            (i > 0) ? lower[i] * expected[i - 1] : 0.0;
        const double center = diagonal[i] * expected[i];
        const double right =
            (i < n - 1) ? upper[i] * expected[i + 1] : 0.0;
        rhs[i] = left + center + right;
    }

    return rhs;
}

void build_diagonally_dominant_system(
    int n,
    std::vector<double>& lower,
    std::vector<double>& diagonal,
    std::vector<double>& upper,
    std::vector<double>& expected
) {
    lower.assign(n, 0.0);
    diagonal.assign(n, 0.0);
    upper.assign(n, 0.0);
    expected.assign(n, 0.0);

    for (int i = 0; i < n; ++i) {
        lower[i] = (i == 0) ? 0.0 : -0.4;
        diagonal[i] = 4.0 + 0.02 * static_cast<double>(i);
        upper[i] = (i == n - 1) ? 0.0 : -0.3;
        expected[i] = 1.0 + 0.07 * static_cast<double>(i);
    }
}

bool test_arbitrary_sizes() {
    const std::vector<int> sizes = {
        1, 2, 3, 4, 5, 7, 8, 15, 16, 31
    };

    for (const int n : sizes) {
        std::vector<double> lower;
        std::vector<double> diagonal;
        std::vector<double> upper;
        std::vector<double> expected;
        build_diagonally_dominant_system(
            n,
            lower,
            diagonal,
            upper,
            expected);

        const std::vector<double> rhs =
            compute_rhs(lower, diagonal, upper, expected);

        std::vector<double> thomas_solution;
        std::vector<double> cr_solution;
        std::vector<double> dispatcher_solution;

        if (!solve_thomas(
                lower,
                diagonal,
                upper,
                rhs,
                thomas_solution)) {
            return false;
        }

        if (!solve_cyclic_reduction(
                lower,
                diagonal,
                upper,
                rhs,
                cr_solution)) {
            return false;
        }

        if (!solve_tridiagonal(
                "cr",
                lower,
                diagonal,
                upper,
                rhs,
                dispatcher_solution)) {
            return false;
        }

        if (!vectors_nearly_equal(cr_solution, expected)) {
            return false;
        }

        if (!vectors_nearly_equal(cr_solution, thomas_solution)) {
            return false;
        }

        if (!vectors_nearly_equal(dispatcher_solution, cr_solution)) {
            return false;
        }
    }

    return true;
}

bool test_invalid_inputs() {
    std::vector<double> solution;

    if (solve_cyclic_reduction({}, {}, {}, {}, solution)) {
        return false;
    }

    const std::vector<double> lower = {0.0, 0.0};
    const std::vector<double> diagonal = {1.0};
    const std::vector<double> upper = {0.0, 0.0};
    const std::vector<double> rhs = {1.0, 1.0};

    if (solve_cyclic_reduction(lower, diagonal, upper, rhs, solution)) {
        return false;
    }

    const std::vector<double> zero_pivot_lower = {0.0, 1.0};
    const std::vector<double> zero_pivot_diagonal = {0.0, 1.0};
    const std::vector<double> zero_pivot_upper = {1.0, 0.0};
    const std::vector<double> zero_pivot_rhs = {1.0, 1.0};

    if (solve_cyclic_reduction(
            zero_pivot_lower,
            zero_pivot_diagonal,
            zero_pivot_upper,
            zero_pivot_rhs,
            solution)) {
        return false;
    }

    return true;
}

}  // namespace

int main() {
    if (!test_arbitrary_sizes()) {
        std::cerr << "test_cyclic_reduction: arbitrary sizes failed\n";
        return 1;
    }

    if (!test_invalid_inputs()) {
        std::cerr << "test_cyclic_reduction: invalid input handling failed\n";
        return 1;
    }

    std::cout << "test_cyclic_reduction: passed\n";
    return 0;
}
