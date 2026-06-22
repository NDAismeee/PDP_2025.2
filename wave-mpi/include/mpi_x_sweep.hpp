#pragma once

#include <mpi.h>

#include "common_types.hpp"
#include "grid.hpp"

bool mpi_x_sweep(
    const Grid& u_previous,
    const Grid& u_current,
    Grid& u_half,
    const SimulationConfig& config,
    const Decomposition& decomposition,
    MPI_Comm communicator,
    TimingInfo& timing
);
