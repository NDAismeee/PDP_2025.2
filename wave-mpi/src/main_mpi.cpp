#include <mpi.h>

#include <iostream>
#include <string>
#include <vector>

#include "common_types.hpp"
#include "decomposition.hpp"
#include "grid.hpp"
#include "metrics.hpp"
#include "mpi_x_sweep.hpp"
#include "mpi_y_sweep.hpp"
#include "result_writer.hpp"
#include "wave_problem.hpp"

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = 0;
    int world_size = 1;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    SimulationConfig config;

    (void)argv;

    const Decomposition decomposition =
        create_block_decomposition(
            config.grid_size,
            rank,
            world_size
        );

    Grid u_previous;
    Grid u_current;
    Grid u_half;
    Grid u_next;

    initialize_wave_problem(
        u_previous,
        u_current,
        config
    );

    u_half.resize(
        config.grid_size,
        config.grid_size
    );

    u_next.resize(
        config.grid_size,
        config.grid_size
    );

    TimingInfo local_timing;

    MPI_Barrier(MPI_COMM_WORLD);
    const double total_start = MPI_Wtime();

    bool success = true;

    for (int step = 0; step < config.time_steps; ++step) {
        success = mpi_x_sweep(
            u_previous,
            u_current,
            u_half,
            config,
            decomposition,
            MPI_COMM_WORLD,
            local_timing
        );

        if (!success) {
            break;
        }

        success = mpi_y_sweep(
            u_half,
            u_current,
            u_next,
            config,
            decomposition,
            MPI_COMM_WORLD,
            local_timing
        );

        if (!success) {
            break;
        }

        u_previous = u_current;
        u_current = u_next;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    local_timing.total = MPI_Wtime() - total_start;

    double global_total = 0.0;

    MPI_Reduce(
        &local_timing.total,
        &global_total,
        1,
        MPI_DOUBLE,
        MPI_MAX,
        0,
        MPI_COMM_WORLD
    );

    if (rank == 0) {
        std::cout << "mode=mpi\n";
        std::cout << "grid_size=" << config.grid_size << "\n";
        std::cout << "time_steps=" << config.time_steps << "\n";
        std::cout << "processes=" << world_size << "\n";
        std::cout << "solver=" << config.solver << "\n";
        std::cout << "communication="
                  << config.communication << "\n";
        std::cout << "status="
                  << (success ? "success" : "failed")
                  << "\n";
        std::cout << "total_seconds="
                  << global_total << "\n";
    }

    MPI_Finalize();
    return success ? 0 : 1;
}
