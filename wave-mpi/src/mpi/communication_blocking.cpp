#include "mpi_communication.hpp"

bool gather_distributed_data_nonblocking(
    const std::vector<double>& local_buffer,
    std::vector<double>& global_buffer,
    const std::vector<int>& counts,
    const std::vector<int>& displacements,
    MPI_Comm communicator,
    double& communication_time
);

bool gather_distributed_data(
    const std::vector<double>& local_buffer,
    std::vector<double>& global_buffer,
    const std::vector<int>& counts,
    const std::vector<int>& displacements,
    const std::string& communication_mode,
    MPI_Comm communicator,
    double& communication_time
) {
    int total = 0;
    for (int count : counts) {
        total += count;
    }

    global_buffer.assign(total, 0.0);

    if (communication_mode == "nonblocking") {
        return gather_distributed_data_nonblocking(
            local_buffer,
            global_buffer,
            counts,
            displacements,
            communicator,
            communication_time
        );
    }

    MPI_Barrier(communicator);
    const double start = MPI_Wtime();

    const int rc = MPI_Allgatherv(
        local_buffer.data(),
        static_cast<int>(local_buffer.size()),
        MPI_DOUBLE,
        global_buffer.data(),
        const_cast<int*>(counts.data()),
        const_cast<int*>(displacements.data()),
        MPI_DOUBLE,
        communicator
    );

    communication_time += MPI_Wtime() - start;
    return rc == MPI_SUCCESS;
}
