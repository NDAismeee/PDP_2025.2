#pragma once

#include <mpi.h>

#include "common_types.hpp"
#include "grid.hpp"

bool mpi_y_sweep(
    const Grid& u_half,
    const Grid& u_current,
    Grid& u_next,
    const SimulationConfig& config,
    const Decomposition& decomposition,
    MPI_Comm communicator,
    TimingInfo& timing
);
