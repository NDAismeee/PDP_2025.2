#include "mpi_y_sweep.hpp"

#include "column_buffer.hpp"
#include "mpi_communication.hpp"
#include "tridiagonal_solver.hpp"
#include "wave_problem.hpp"

#include <mpi.h>
#include <cmath>

bool mpi_y_sweep(
    const Grid& u_half,
    const Grid& u_current,
    Grid& u_next,
    const SimulationConfig& config,
    const Decomposition& decomposition,
    MPI_Comm communicator,
    TimingInfo& timing
) {
    const int N = config.grid_size;
    const int rows = N;
    const int cols = N;

    const int local_start = decomposition.local_start;
    const int local_count = decomposition.local_count;

    if (local_count <= 0) {
        return true;
    }

    const double dx = config.domain_length / static_cast<double>(N - 1);
    const double dy = config.domain_length / static_cast<double>(N - 1);
    const double dt = config.total_time / static_cast<double>(config.time_steps);
    const double c = config.wave_speed;
    const double alpha = (c * dt) / (2.0 * dy);
    const double alpha_sq = alpha * alpha;

    std::vector<double> local_buffer;
    pack_columns(u_half, local_start, local_count, local_buffer);

    const double compute_start = MPI_Wtime();

    std::vector<double> lower(rows, -alpha_sq);
    std::vector<double> diagonal(rows, 1.0 + 2.0 * alpha_sq);
    std::vector<double> upper(rows, -alpha_sq);
    std::vector<double> rhs(rows);
    std::vector<double> solution(rows);

    for (int local_col = 0; local_col < local_count; ++local_col) {
        const int global_col = local_start + local_col;

        for (int row = 0; row < rows; ++row) {
            if (row == 0 || row == rows - 1 || global_col == 0 || global_col == cols - 1) {
                rhs[row] = 0.0;
            } else {
                rhs[row] = u_half(row, global_col) + alpha_sq * (u_current(row - 1, global_col) - 2.0 * u_current(row, global_col) + u_current(row + 1, global_col));
            }
        }

        const bool success = solve_tridiagonal(
            config.solver,
            lower,
            diagonal,
            upper,
            rhs,
            solution
        );

        if (!success) {
            return false;
        }

        for (int row = 0; row < rows; ++row) {
            local_buffer[local_col * rows + row] = solution[row];
        }
    }

    timing.compute_y += MPI_Wtime() - compute_start;

    std::vector<double> global_buffer;

    const bool comm_success = gather_distributed_data(
        local_buffer,
        global_buffer,
        decomposition.counts,
        decomposition.displacements,
        config.communication,
        communicator,
        timing.communication_y
    );

    if (!comm_success) {
        return false;
    }

    unpack_columns(global_buffer, 0, cols, u_next);

    apply_boundary_conditions(u_next);

    return true;
}
