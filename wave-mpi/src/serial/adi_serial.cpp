#include "adi_serial.hpp"

#include "tridiagonal_solver.hpp"
#include "wave_problem.hpp"

#include <chrono>
#include <cmath>
#include <utility>
#include <vector>

namespace {

bool is_valid_config(const SimulationConfig& config) {
    if (config.grid_size < 3) {
        return false;
    }
    if (config.time_steps < 1) {
        return false;
    }
    if (config.domain_length <= 0.0) {
        return false;
    }
    if (config.total_time <= 0.0) {
        return false;
    }
    if (config.wave_speed <= 0.0) {
        return false;
    }
    if (config.solver != "thomas" && config.solver != "cr") {
        return false;
    }
    return true;
}

bool grids_match_config(
    const Grid& grid_a,
    const Grid& grid_b,
    int expected_size
) {
    return grid_a.rows() == expected_size &&
           grid_a.cols() == expected_size &&
           grid_b.rows() == expected_size &&
           grid_b.cols() == expected_size;
}

double compute_mu(const SimulationConfig& config) {
    const int n = config.grid_size;
    const double h =
        config.domain_length / static_cast<double>(n - 1);
    const double dt =
        config.total_time / static_cast<double>(config.time_steps);
    const double ratio = config.wave_speed * dt / h;
    return 0.5 * ratio * ratio;
}

}  // namespace

bool serial_x_sweep(
    const Grid& u_previous,
    const Grid& u_current,
    Grid& u_half,
    const SimulationConfig& config,
    TimingInfo& timing
) {
    const auto start = std::chrono::steady_clock::now();

    if (!is_valid_config(config)) {
        return false;
    }

    const int n = config.grid_size;
    if (!grids_match_config(u_previous, u_current, n)) {
        return false;
    }

    u_half.resize(n, n);
    u_half.fill(0.0);

    const int interior_size = n - 2;
    const double mu = compute_mu(config);

    std::vector<double> lower(interior_size, -mu);
    std::vector<double> diagonal(interior_size, 1.0 + 2.0 * mu);
    std::vector<double> upper(interior_size, -mu);
    lower.front() = 0.0;
    upper.back() = 0.0;

    std::vector<double> rhs(interior_size);
    std::vector<double> solution(interior_size);

    for (int row = 1; row < n - 1; ++row) {
        for (int col = 1; col < n - 1; ++col) {
            const double dx_previous =
                u_previous(row, col + 1) -
                2.0 * u_previous(row, col) +
                u_previous(row, col - 1);

            const double dy_previous =
                u_previous(row + 1, col) -
                2.0 * u_previous(row, col) +
                u_previous(row - 1, col);

            rhs[col - 1] =
                2.0 * u_current(row, col) -
                u_previous(row, col) +
                mu * (dx_previous + dy_previous);
        }

        if (!solve_tridiagonal(
                config.solver,
                lower,
                diagonal,
                upper,
                rhs,
                solution)) {
            return false;
        }

        for (int col = 1; col < n - 1; ++col) {
            u_half(row, col) = solution[col - 1];
        }
    }

    apply_boundary_conditions(u_half);

    const auto end = std::chrono::steady_clock::now();
    timing.compute_x +=
        std::chrono::duration<double>(end - start).count();

    return true;
}

bool serial_y_sweep(
    const Grid& u_half,
    const Grid& u_current,
    Grid& u_next,
    const SimulationConfig& config,
    TimingInfo& timing
) {
    (void)u_current;

    const auto start = std::chrono::steady_clock::now();

    if (!is_valid_config(config)) {
        return false;
    }

    const int n = config.grid_size;
    if (u_half.rows() != n || u_half.cols() != n) {
        return false;
    }

    u_next.resize(n, n);
    u_next.fill(0.0);

    const int interior_size = n - 2;
    const double mu = compute_mu(config);

    std::vector<double> lower(interior_size, -mu);
    std::vector<double> diagonal(interior_size, 1.0 + 2.0 * mu);
    std::vector<double> upper(interior_size, -mu);
    lower.front() = 0.0;
    upper.back() = 0.0;

    std::vector<double> rhs(interior_size);
    std::vector<double> solution(interior_size);

    for (int col = 1; col < n - 1; ++col) {
        for (int row = 1; row < n - 1; ++row) {
            rhs[row - 1] = u_half(row, col);
        }

        if (!solve_tridiagonal(
                config.solver,
                lower,
                diagonal,
                upper,
                rhs,
                solution)) {
            return false;
        }

        for (int row = 1; row < n - 1; ++row) {
            u_next(row, col) = solution[row - 1];
        }
    }

    apply_boundary_conditions(u_next);

    const auto end = std::chrono::steady_clock::now();
    timing.compute_y +=
        std::chrono::duration<double>(end - start).count();

    return true;
}

bool solve_wave_serial(
    const SimulationConfig& config,
    Grid& final_result,
    TimingInfo& timing
) {
    timing = TimingInfo{};

    if (!is_valid_config(config)) {
        return false;
    }

    const auto total_start = std::chrono::steady_clock::now();

    const int n = config.grid_size;
    Grid u_previous;
    Grid u_current;

    const auto init_start = std::chrono::steady_clock::now();
    initialize_wave_problem(u_previous, u_current, config);
    const auto init_end = std::chrono::steady_clock::now();
    timing.initialization =
        std::chrono::duration<double>(init_end - init_start).count();

    if (config.time_steps == 1) {
        final_result = u_current;
        const auto total_end = std::chrono::steady_clock::now();
        timing.total =
            std::chrono::duration<double>(total_end - total_start).count();
        return true;
    }

    Grid u_half(n, n);
    Grid u_next(n, n);

    for (int step = 1; step < config.time_steps; ++step) {
        (void)step;

        if (!serial_x_sweep(
                u_previous,
                u_current,
                u_half,
                config,
                timing)) {
            return false;
        }

        if (!serial_y_sweep(
                u_half,
                u_current,
                u_next,
                config,
                timing)) {
            return false;
        }

        std::swap(u_previous, u_current);
        std::swap(u_current, u_next);
    }

    final_result = u_current;

    const auto total_end = std::chrono::steady_clock::now();
    timing.total =
        std::chrono::duration<double>(total_end - total_start).count();

    return true;
}
