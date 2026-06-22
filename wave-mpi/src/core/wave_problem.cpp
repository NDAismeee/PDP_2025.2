#include "wave_problem.hpp"

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void apply_boundary_conditions(Grid& grid) {
    const int rows = grid.rows();
    const int cols = grid.cols();

    for (int col = 0; col < cols; ++col) {
        grid(0, col) = 0.0;
        grid(rows - 1, col) = 0.0;
    }

    for (int row = 0; row < rows; ++row) {
        grid(row, 0) = 0.0;
        grid(row, cols - 1) = 0.0;
    }
}

void initialize_wave_problem(
    Grid& u_previous,
    Grid& u_current,
    const SimulationConfig& config
) {
    const int n = config.grid_size;
    const double h = config.domain_length / (n - 1);

    u_previous.resize(n, n);
    u_current.resize(n, n);

    for (int row = 0; row < n; ++row) {
        for (int col = 0; col < n; ++col) {
            const double x = col * h;
            const double y = row * h;
            const double value =
                std::sin(M_PI * x / config.domain_length) *
                std::sin(M_PI * y / config.domain_length);

            u_previous(row, col) = value;
            u_current(row, col) = value;
        }
    }

    apply_boundary_conditions(u_previous);
    apply_boundary_conditions(u_current);
}

double exact_solution(
    double t,
    double x,
    double y,
    const SimulationConfig& config
) {
    const double omega =
        config.wave_speed * M_PI * std::sqrt(2.0) / config.domain_length;

    return std::sin(M_PI * x / config.domain_length) *
           std::sin(M_PI * y / config.domain_length) *
           std::cos(omega * t);
}
