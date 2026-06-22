#pragma once

#include <mpi.h>

#include <string>
#include <vector>

bool gather_distributed_data(
    const std::vector<double>& local_buffer,
    std::vector<double>& global_buffer,
    const std::vector<int>& counts,
    const std::vector<int>& displacements,
    const std::string& communication_mode,
    MPI_Comm communicator,
    double& communication_time
);
