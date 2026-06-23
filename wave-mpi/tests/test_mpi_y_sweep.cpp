#include <cmath>
#include <iostream>
#include <mpi.h>

#include "adi_serial.hpp"
#include "common_types.hpp"
#include "decomposition.hpp"
#include "grid.hpp"
#include "mpi_y_sweep.hpp"
#include "wave_problem.hpp"

int main() {
    MPI_Init(nullptr, nullptr);

    int rank = 0;
    int world_size = 1;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    const int N = 127;
    SimulationConfig config;
    config.grid_size = N;
    config.time_steps = 10;
    config.domain_length = 1.0;
    config.total_time = 0.1;
    config.wave_speed = 1.0;
    config.solver = "thomas";

    Grid u_prev(N, N);
    Grid u_curr(N, N);
    Grid u_half(N, N);
    Grid u_next_mpi(N, N);
    Grid u_next_serial(N, N);

    initialize_wave_problem(u_prev, u_curr, config);

    const Decomposition decomposition = create_block_decomposition(N, rank, world_size);

    TimingInfo mpi_timing;
    const bool mpi_success = mpi_y_sweep(
        u_curr,
        u_prev,
        u_next_mpi,
        config,
        decomposition,
        MPI_COMM_WORLD,
        mpi_timing
    );

    if (!mpi_success) {
        if (rank == 0) {
            std::cerr << "test_mpi_y_sweep: mpi_y_sweep failed\n";
        }
        MPI_Finalize();
        return 1;
    }

    TimingInfo serial_timing;
    const bool serial_success = serial_y_sweep(
        u_curr,
        u_prev,
        u_next_serial,
        config,
        serial_timing
    );

    if (!serial_success) {
        if (rank == 0) {
            std::cerr << "test_mpi_y_sweep: serial_y_sweep failed\n";
        }
        MPI_Finalize();
        return 1;
    }

    if (rank == 0) {
        double max_error = 0.0;
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                const double error = std::abs(u_next_mpi(i, j) - u_next_serial(i, j));
                if (error > max_error) {
                    max_error = error;
                }
            }
        }

        if (max_error < 1e-8) {
            std::cout << "test_mpi_y_sweep: y_sweep_matches_serial status=passed max_error=" << max_error << "\n";
        } else {
            std::cerr << "test_mpi_y_sweep: y_sweep_matches_serial status=failed max_error=" << max_error << "\n";
            MPI_Finalize();
            return 1;
        }
    }

    config.communication = "nonblocking";
    Grid u_next_nonblocking(N, N);

    TimingInfo nonblock_timing;
    const bool nonblock_success = mpi_y_sweep(
        u_curr,
        u_prev,
        u_next_nonblocking,
        config,
        decomposition,
        MPI_COMM_WORLD,
        nonblock_timing
    );

    if (!nonblock_success) {
        if (rank == 0) {
            std::cerr << "test_mpi_y_sweep: mpi_y_sweep nonblocking failed\n";
        }
        MPI_Finalize();
        return 1;
    }

    if (rank == 0) {
        double max_error_nonblock = 0.0;
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                const double error = std::abs(u_next_nonblocking(i, j) - u_next_mpi(i, j));
                if (error > max_error_nonblock) {
                    max_error_nonblock = error;
                }
            }
        }

        if (max_error_nonblock < 1e-10) {
            std::cout << "test_mpi_y_sweep: blocking_nonblocking_equivalence status=passed max_error=" << max_error_nonblock << "\n";
        } else {
            std::cerr << "test_mpi_y_sweep: blocking_nonblocking_equivalence status=failed max_error=" << max_error_nonblock << "\n";
            MPI_Finalize();
            return 1;
        }

        std::cout << "test_mpi_y_sweep: all tests passed\n";
    }

    MPI_Finalize();
    return 0;
}
