#include <cmath>
#include <iostream>

#include "adi_serial.hpp"
#include "wave_problem.hpp"

namespace {

double max_grid_error(const Grid& actual, const Grid& expected) {
    double max_error = 0.0;
    const int rows = actual.rows();
    const int cols = actual.cols();

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            max_error = std::max(
                max_error,
                std::abs(actual(row, col) - expected(row, col)));
        }
    }

    return max_error;
}

double max_analytic_error(
    const Grid& result,
    const SimulationConfig& config
) {
    const int n = config.grid_size;
    const double h =
        config.domain_length / static_cast<double>(n - 1);
    const double t = config.total_time;

    double max_error = 0.0;
    for (int row = 0; row < n; ++row) {
        for (int col = 0; col < n; ++col) {
            const double x = col * h;
            const double y = row * h;
            const double exact =
                exact_solution(t, x, y, config);
            max_error = std::max(
                max_error,
                std::abs(result(row, col) - exact));
        }
    }

    return max_error;
}

bool has_finite_values(const Grid& grid) {
    for (int row = 0; row < grid.rows(); ++row) {
        for (int col = 0; col < grid.cols(); ++col) {
            const double value = grid(row, col);
            if (!std::isfinite(value)) {
                return false;
            }
        }
    }
    return true;
}

bool boundary_is_zero(const Grid& grid) {
    const int rows = grid.rows();
    const int cols = grid.cols();

    for (int col = 0; col < cols; ++col) {
        if (grid(0, col) != 0.0 || grid(rows - 1, col) != 0.0) {
            return false;
        }
    }

    for (int row = 0; row < rows; ++row) {
        if (grid(row, 0) != 0.0 || grid(row, cols - 1) != 0.0) {
            return false;
        }
    }

    return true;
}

bool test_thomas_serial_run() {
    SimulationConfig config;
    config.grid_size = 31;
    config.time_steps = 100;
    config.domain_length = 1.0;
    config.total_time = 0.2;
    config.wave_speed = 1.0;
    config.solver = "thomas";

    Grid result;
    TimingInfo timing;

    if (!solve_wave_serial(config, result, timing)) {
        return false;
    }

    if (result.rows() != 31 || result.cols() != 31) {
        return false;
    }

    if (!has_finite_values(result)) {
        return false;
    }

    if (!boundary_is_zero(result)) {
        return false;
    }

    if (timing.total <= 0.0 ||
        timing.compute_x <= 0.0 ||
        timing.compute_y <= 0.0) {
        return false;
    }

    if (timing.communication_x != 0.0 ||
        timing.communication_y != 0.0) {
        return false;
    }

    return true;
}

bool test_exact_solution_error() {
    SimulationConfig config;
    config.grid_size = 31;
    config.time_steps = 100;
    config.domain_length = 1.0;
    config.total_time = 0.2;
    config.wave_speed = 1.0;
    config.solver = "thomas";

    Grid result;
    TimingInfo timing;

    if (!solve_wave_serial(config, result, timing)) {
        return false;
    }

    const double max_error = max_analytic_error(result, config);
    return max_error < 2e-3;
}

bool test_thomas_vs_cr() {
    SimulationConfig thomas_config;
    thomas_config.grid_size = 21;
    thomas_config.time_steps = 40;
    thomas_config.domain_length = 1.0;
    thomas_config.total_time = 0.1;
    thomas_config.wave_speed = 1.0;
    thomas_config.solver = "thomas";

    SimulationConfig cr_config = thomas_config;
    cr_config.solver = "cr";

    Grid thomas_result;
    Grid cr_result;
    TimingInfo thomas_timing;
    TimingInfo cr_timing;

    if (!solve_wave_serial(thomas_config, thomas_result, thomas_timing)) {
        return false;
    }

    if (!solve_wave_serial(cr_config, cr_result, cr_timing)) {
        return false;
    }

    return max_grid_error(thomas_result, cr_result) < 1e-8;
}

bool test_invalid_config() {
    Grid result;
    TimingInfo timing;

    SimulationConfig invalid_grid;
    invalid_grid.grid_size = 2;
    if (solve_wave_serial(invalid_grid, result, timing)) {
        return false;
    }

    SimulationConfig invalid_steps;
    invalid_steps.time_steps = 0;
    if (solve_wave_serial(invalid_steps, result, timing)) {
        return false;
    }

    SimulationConfig invalid_domain;
    invalid_domain.domain_length = 0.0;
    if (solve_wave_serial(invalid_domain, result, timing)) {
        return false;
    }

    SimulationConfig invalid_time;
    invalid_time.total_time = -1.0;
    if (solve_wave_serial(invalid_time, result, timing)) {
        return false;
    }

    SimulationConfig invalid_solver;
    invalid_solver.solver = "unknown";
    if (solve_wave_serial(invalid_solver, result, timing)) {
        return false;
    }

    return true;
}

}  // namespace

int main() {
    if (!test_thomas_serial_run()) {
        std::cerr << "test_serial_solver: thomas serial run failed\n";
        return 1;
    }

    if (!test_exact_solution_error()) {
        std::cerr << "test_serial_solver: analytic comparison failed\n";
        return 1;
    }

    if (!test_thomas_vs_cr()) {
        std::cerr << "test_serial_solver: thomas vs cr failed\n";
        return 1;
    }

    if (!test_invalid_config()) {
        std::cerr << "test_serial_solver: invalid config handling failed\n";
        return 1;
    }

    std::cout << "test_serial_solver: passed\n";
    return 0;
}
