#include "mpi_x_sweep.hpp"

bool mpi_x_sweep(
    const Grid& u_previous,
    const Grid& u_current,
    Grid& u_half,
    const SimulationConfig& config,
    const Decomposition& decomposition,
    MPI_Comm communicator,
    TimingInfo& timing
) {
    (void)u_previous;
    (void)u_current;
    (void)u_half;
    (void)config;
    (void)decomposition;
    (void)communicator;
    (void)timing;
    return false;
}
