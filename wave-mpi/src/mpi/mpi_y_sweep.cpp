#include "mpi_y_sweep.hpp"

bool mpi_y_sweep(
    const Grid& u_half,
    const Grid& u_current,
    Grid& u_next,
    const SimulationConfig& config,
    const Decomposition& decomposition,
    MPI_Comm communicator,
    TimingInfo& timing
) {
    (void)u_half;
    (void)u_current;
    (void)u_next;
    (void)config;
    (void)decomposition;
    (void)communicator;
    (void)timing;
    return false;
}
