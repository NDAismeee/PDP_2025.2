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

    const double dy = config.domain_length / static_cast<double>(N - 1);
    const double dt = config.total_time / static_cast<double>(config.time_steps);
    const double c = config.wave_speed;
    (void)dy;
    (void)dt;
    (void)c;

    std::vector<double> local_buffer;
    pack_columns(u_half, local_start, local_count, local_buffer);

    const double compute_start = MPI_Wtime();

    const double h = config.domain_length / static_cast<double>(N - 1);
    const double ratio = c * dt / h;
    const double mu = 0.5 * ratio * ratio;

    const int interior_size = N - 2;
    std::vector<double> lower(interior_size, -mu);
    std::vector<double> diagonal(interior_size, 1.0 + 2.0 * mu);
    std::vector<double> upper(interior_size, -mu);
    lower.front() = 0.0;
    upper.back() = 0.0;
    std::vector<double> rhs(interior_size);
    std::vector<double> solution(interior_size);

    for (int local_col = 0; local_col < local_count; ++local_col) {
        const int global_col = local_start + local_col;

        if (global_col == 0 || global_col == cols - 1) {
            // Boundary column: all zeros
            for (int row = 1; row < rows - 1; ++row) {
                local_buffer[local_col * rows + row] = 0.0;
            }
        } else {
            // Interior column: solve tridiagonal
            for (int row = 1; row < rows - 1; ++row) {
                rhs[row - 1] = u_half(row, global_col);
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

            for (int row = 1; row < rows - 1; ++row) {
                local_buffer[local_col * rows + row] = solution[row - 1];
            }
        }
    }

    timing.compute_y += MPI_Wtime() - compute_start;

    // Use MPI_Allgatherv to gather all columns
    int rank;
    int world_size;
    MPI_Comm_rank(communicator, &rank);
    MPI_Comm_size(communicator, &world_size);

    std::vector<int> recvcounts(world_size);
    std::vector<int> displs(world_size);

    for (int r = 0; r < world_size; ++r) {
        recvcounts[r] = decomposition.counts[r] * rows;
        displs[r] = decomposition.displacements[r] * rows;
    }

    // Allocate global buffer
    std::vector<double> global_buffer(N * rows, 0.0);

    const double comm_start = MPI_Wtime();

    const int rc = MPI_Allgatherv(
        local_buffer.data(),
        static_cast<int>(local_buffer.size()),
        MPI_DOUBLE,
        global_buffer.data(),
        recvcounts.data(),
        displs.data(),
        MPI_DOUBLE,
        communicator
    );

    timing.communication_y += MPI_Wtime() - comm_start;

    if (rc != MPI_SUCCESS) {
        return false;
    }

    unpack_columns(global_buffer, 0, cols, u_next);

    apply_boundary_conditions(u_next);

    return true;
}
