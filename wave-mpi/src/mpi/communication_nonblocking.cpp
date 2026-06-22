#include "mpi_communication.hpp"

bool gather_distributed_data_nonblocking(
    const std::vector<double>& local_buffer,
    std::vector<double>& global_buffer,
    const std::vector<int>& counts,
    const std::vector<int>& displacements,
    MPI_Comm communicator,
    double& communication_time
) {
    int total = 0;
    for (int count : counts) {
        total += count;
    }

    global_buffer.assign(total, 0.0);

    MPI_Barrier(communicator);
    const double start = MPI_Wtime();

    MPI_Request request = MPI_REQUEST_NULL;
    const int rc = MPI_Iallgatherv(
        local_buffer.data(),
        static_cast<int>(local_buffer.size()),
        MPI_DOUBLE,
        global_buffer.data(),
        const_cast<int*>(counts.data()),
        const_cast<int*>(displacements.data()),
        MPI_DOUBLE,
        communicator,
        &request
    );

    if (rc != MPI_SUCCESS) {
        return false;
    }

    MPI_Wait(&request, MPI_STATUS_IGNORE);
    communication_time += MPI_Wtime() - start;
    return true;
}
