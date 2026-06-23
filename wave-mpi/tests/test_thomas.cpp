#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "tridiagonal_solver.hpp"

namespace {

constexpr double kTolerance = 1e-10;

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

bool test_1x1_system() {
    const std::vector<double> lower = {0.0};
    const std::vector<double> diagonal = {2.0};
    const std::vector<double> upper = {0.0};
    const std::vector<double> rhs = {8.0};
    const std::vector<double> expected = {4.0};

    std::vector<double> solution;
    if (!solve_thomas(lower, diagonal, upper, rhs, solution)) {
        return false;
    }

    return vectors_nearly_equal(solution, expected);
}

bool test_3x3_known_solution() {
    const std::vector<double> lower = {0.0, -1.0, 0.0};
    const std::vector<double> diagonal = {2.0, 2.0, 2.0};
    const std::vector<double> upper = {-1.0, -1.0, 0.0};
    const std::vector<double> expected = {1.0, 2.0, 3.0};
    const std::vector<double> rhs =
        compute_rhs(lower, diagonal, upper, expected);

    std::vector<double> solution;
    if (!solve_thomas(lower, diagonal, upper, rhs, solution)) {
        return false;
    }

    return vectors_nearly_equal(solution, expected);
}

bool test_size_10_diagonally_dominant() {
    const int n = 10;
    std::vector<double> lower(n);
    std::vector<double> diagonal(n);
    std::vector<double> upper(n);
    std::vector<double> expected(n);

    for (int i = 0; i < n; ++i) {
        lower[i] = (i == 0) ? 0.0 : -0.5;
        diagonal[i] = 3.0 + 0.01 * static_cast<double>(i);
        upper[i] = (i == n - 1) ? 0.0 : -0.25;
        expected[i] = 0.5 + 0.1 * static_cast<double>(i);
    }

    const std::vector<double> rhs =
        compute_rhs(lower, diagonal, upper, expected);

    std::vector<double> solution;
    if (!solve_thomas(lower, diagonal, upper, rhs, solution)) {
        return false;
    }

    return vectors_nearly_equal(solution, expected);
}

bool test_invalid_inputs() {
    std::vector<double> solution;

    if (solve_thomas({}, {}, {}, {}, solution)) {
        return false;
    }

    const std::vector<double> lower = {0.0, 0.0};
    const std::vector<double> diagonal = {1.0};
    const std::vector<double> upper = {0.0, 0.0};
    const std::vector<double> rhs = {1.0, 1.0};

    if (solve_thomas(lower, diagonal, upper, rhs, solution)) {
        return false;
    }

    const std::vector<double> zero_pivot_lower = {0.0, 1.0};
    const std::vector<double> zero_pivot_diagonal = {0.0, 1.0};
    const std::vector<double> zero_pivot_upper = {1.0, 0.0};
    const std::vector<double> zero_pivot_rhs = {1.0, 1.0};

    if (solve_thomas(
            zero_pivot_lower,
            zero_pivot_diagonal,
            zero_pivot_upper,
            zero_pivot_rhs,
            solution)) {
        return false;
    }

    const std::vector<double> valid_lower = {0.0, -1.0, 0.0};
    const std::vector<double> valid_diagonal = {2.0, 2.0, 2.0};
    const std::vector<double> valid_upper = {-1.0, -1.0, 0.0};
    const std::vector<double> valid_rhs = {0.0, 0.0, 6.0};

    if (solve_tridiagonal(
            "unknown",
            valid_lower,
            valid_diagonal,
            valid_upper,
            valid_rhs,
            solution)) {
        return false;
    }

    return true;
}

}  // namespace

int main() {
    if (!test_1x1_system()) {
        std::cerr << "test_thomas: 1x1 system failed\n";
        return 1;
    }

    if (!test_3x3_known_solution()) {
        std::cerr << "test_thomas: 3x3 system failed\n";
        return 1;
    }

    if (!test_size_10_diagonally_dominant()) {
        std::cerr << "test_thomas: size 10 system failed\n";
        return 1;
    }

    if (!test_invalid_inputs()) {
        std::cerr << "test_thomas: invalid input handling failed\n";
        return 1;
    }

    std::cout << "test_thomas: passed\n";
    return 0;
}
