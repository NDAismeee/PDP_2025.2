#include <cmath>
#include <iostream>
#include <mpi.h>
#include <vector>

#include "mpi_communication.hpp"

int main() {
    MPI_Init(nullptr, nullptr);

    int rank = 0;
    int world_size = 1;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    const int local_size = 10;
    std::vector<double> local_data(local_size);

    for (int i = 0; i < local_size; ++i) {
        local_data[i] = static_cast<double>(rank * 100 + i);
    }

    std::vector<int> counts(world_size);
    std::vector<int> displacements(world_size);

    int total_size = 0;
    for (int i = 0; i < world_size; ++i) {
        counts[i] = local_size;
        displacements[i] = i * local_size;
        total_size += local_size;
    }

    std::vector<double> global_data_blocking;
    double comm_time_blocking = 0.0;

    const bool blocking_success = gather_distributed_data(
        local_data,
        global_data_blocking,
        counts,
        displacements,
        "blocking",
        MPI_COMM_WORLD,
        comm_time_blocking
    );

    if (!blocking_success) {
        if (rank == 0) {
            std::cerr << "test_communication: blocking gather failed\n";
        }
        MPI_Finalize();
        return 1;
    }

    if (rank == 0) {
        bool data_correct = true;
        for (int i = 0; i < world_size; ++i) {
            for (int j = 0; j < local_size; ++j) {
                const int idx = i * local_size + j;
                const double expected = static_cast<double>(i * 100 + j);
                if (std::abs(global_data_blocking[idx] - expected) > 1e-12) {
                    data_correct = false;
                    std::cerr << "test_communication: blocking data mismatch at " << idx
                              << ": got " << global_data_blocking[idx]
                              << ", expected " << expected << "\n";
                }
            }
        }

        if (data_correct) {
            std::cout << "test_communication: blocking_gather status=passed\n";
        } else {
            MPI_Finalize();
            return 1;
        }
    }

    std::vector<double> global_data_nonblocking;
    double comm_time_nonblocking = 0.0;

    const bool nonblocking_success = gather_distributed_data(
        local_data,
        global_data_nonblocking,
        counts,
        displacements,
        "nonblocking",
        MPI_COMM_WORLD,
        comm_time_nonblocking
    );

    if (!nonblocking_success) {
        if (rank == 0) {
            std::cerr << "test_communication: nonblocking gather failed\n";
        }
        MPI_Finalize();
        return 1;
    }

    if (rank == 0) {
        bool data_correct = true;
        for (int i = 0; i < world_size; ++i) {
            for (int j = 0; j < local_size; ++j) {
                const int idx = i * local_size + j;
                const double expected = static_cast<double>(i * 100 + j);
                if (std::abs(global_data_nonblocking[idx] - expected) > 1e-12) {
                    data_correct = false;
                    std::cerr << "test_communication: nonblocking data mismatch at " << idx
                              << ": got " << global_data_nonblocking[idx]
                              << ", expected " << expected << "\n";
                }
            }
        }

        if (data_correct) {
            std::cout << "test_communication: nonblocking_gather status=passed\n";
        } else {
            MPI_Finalize();
            return 1;
        }

        bool modes_equivalent = true;
        for (size_t i = 0; i < global_data_blocking.size(); ++i) {
            if (std::abs(global_data_blocking[i] - global_data_nonblocking[i]) > 1e-12) {
                modes_equivalent = false;
                std::cerr << "test_communication: modes differ at " << i << "\n";
            }
        }

        if (modes_equivalent) {
            std::cout << "test_communication: blocking_nonblocking_equivalence status=passed\n";
        } else {
            MPI_Finalize();
            return 1;
        }

        std::cout << "test_communication: all tests passed\n";
    }

    MPI_Finalize();
    return 0;
}
