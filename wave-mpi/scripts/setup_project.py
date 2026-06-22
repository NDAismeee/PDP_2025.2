#!/usr/bin/env python3
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

DIRS = [
    "config",
    "include",
    "src/core",
    "src/solver",
    "src/serial",
    "src/mpi",
    "src/benchmark",
    "tests",
    "scripts",
    "results/raw",
    "results/summaries",
    "results/figures",
    "docs",
]

FILES = {}

FILES[".gitignore"] = """build/
.venv/
__pycache__/
*.pyc
*.o
*.out
*.log
*.bin

results/raw/*
results/figures/*
results/summaries/*

!results/raw/.gitkeep
!results/figures/.gitkeep
!results/summaries/.gitkeep
"""

FILES["CMakeLists.txt"] = """cmake_minimum_required(VERSION 3.16)

project(wave_mpi LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(MPI REQUIRED)

add_library(wave_core
    src/core/grid.cpp
    src/core/wave_problem.cpp
    src/solver/thomas.cpp
    src/solver/cyclic_reduction.cpp
    src/serial/adi_serial.cpp
    src/mpi/decomposition.cpp
    src/mpi/mpi_x_sweep.cpp
    src/mpi/column_buffer.cpp
    src/mpi/communication_blocking.cpp
    src/mpi/communication_nonblocking.cpp
    src/mpi/mpi_y_sweep.cpp
    src/benchmark/metrics.cpp
    src/benchmark/result_writer.cpp
)

target_include_directories(wave_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(wave_core
    PUBLIC
        MPI::MPI_CXX
)

if(MSVC)
    target_compile_options(wave_core PRIVATE /W4 /O2)
else()
    target_compile_options(wave_core PRIVATE -Wall -Wextra -Wpedantic -O3)
endif()

add_executable(wave_serial src/main_serial.cpp)
target_link_libraries(wave_serial PRIVATE wave_core)

add_executable(wave_mpi src/main_mpi.cpp)
target_link_libraries(wave_mpi PRIVATE wave_core MPI::MPI_CXX)

enable_testing()

add_executable(test_thomas tests/test_thomas.cpp)
target_link_libraries(test_thomas PRIVATE wave_core)
add_test(NAME test_thomas COMMAND test_thomas)

add_executable(test_cyclic_reduction tests/test_cyclic_reduction.cpp)
target_link_libraries(test_cyclic_reduction PRIVATE wave_core)
add_test(NAME test_cyclic_reduction COMMAND test_cyclic_reduction)

add_executable(test_decomposition tests/test_decomposition.cpp)
target_link_libraries(test_decomposition PRIVATE wave_core)
add_test(NAME test_decomposition COMMAND test_decomposition)

add_executable(test_column_buffer tests/test_column_buffer.cpp)
target_link_libraries(test_column_buffer PRIVATE wave_core)
add_test(NAME test_column_buffer COMMAND test_column_buffer)

add_executable(test_metrics tests/test_metrics.cpp)
target_link_libraries(test_metrics PRIVATE wave_core)
add_test(NAME test_metrics COMMAND test_metrics)

add_executable(test_mpi_x_sweep tests/test_mpi_x_sweep.cpp)
target_link_libraries(test_mpi_x_sweep PRIVATE wave_core MPI::MPI_CXX)

add_executable(test_mpi_y_sweep tests/test_mpi_y_sweep.cpp)
target_link_libraries(test_mpi_y_sweep PRIVATE wave_core MPI::MPI_CXX)
"""

FILES["hosts"] = """master slots=4
slave1 slots=4
slave2 slots=4
"""

FILES["config/default.conf"] = """grid_size=255
time_steps=50
domain_length=1.0
total_time=1.0
wave_speed=1.0
solver=thomas
communication=blocking
"""

FILES["include/common_types.hpp"] = """#pragma once

#include <string>
#include <vector>

struct SimulationConfig {
    int grid_size = 255;
    int time_steps = 50;

    double domain_length = 1.0;
    double total_time = 1.0;
    double wave_speed = 1.0;

    std::string solver = "thomas";
    std::string communication = "blocking";

    std::string output_path = "";
    std::string reference_path = "";
};

struct TimingInfo {
    double initialization = 0.0;

    double compute_x = 0.0;
    double communication_x = 0.0;

    double compute_y = 0.0;
    double communication_y = 0.0;

    double waiting = 0.0;
    double total = 0.0;
};

struct ErrorMetrics {
    double max_absolute_error = 0.0;
    double l2_error = 0.0;
};

struct Decomposition {
    int global_size = 0;
    int local_start = 0;
    int local_count = 0;

    std::vector<int> counts;
    std::vector<int> displacements;
};
"""

FILES["include/grid.hpp"] = """#pragma once

#include <vector>

class Grid {
public:
    Grid();
    Grid(int rows, int cols);

    void resize(int rows, int cols);
    void fill(double value);

    double& operator()(int row, int col);
    const double& operator()(int row, int col) const;

    double* data();
    const double* data() const;

    int rows() const;
    int cols() const;
    int size() const;

private:
    int rows_ = 0;
    int cols_ = 0;
    std::vector<double> values_;
};
"""

FILES["include/tridiagonal_solver.hpp"] = """#pragma once

#include <string>
#include <vector>

bool solve_thomas(
    const std::vector<double>& lower,
    const std::vector<double>& diagonal,
    const std::vector<double>& upper,
    const std::vector<double>& rhs,
    std::vector<double>& solution
);

bool solve_cyclic_reduction(
    const std::vector<double>& lower,
    const std::vector<double>& diagonal,
    const std::vector<double>& upper,
    const std::vector<double>& rhs,
    std::vector<double>& solution
);

bool solve_tridiagonal(
    const std::string& solver_name,
    const std::vector<double>& lower,
    const std::vector<double>& diagonal,
    const std::vector<double>& upper,
    const std::vector<double>& rhs,
    std::vector<double>& solution
);
"""

FILES["include/wave_problem.hpp"] = """#pragma once

#include "common_types.hpp"
#include "grid.hpp"

void initialize_wave_problem(
    Grid& u_previous,
    Grid& u_current,
    const SimulationConfig& config
);

void apply_boundary_conditions(Grid& grid);

double exact_solution(
    double t,
    double x,
    double y,
    const SimulationConfig& config
);
"""

FILES["include/adi_serial.hpp"] = """#pragma once

#include "common_types.hpp"
#include "grid.hpp"

bool serial_x_sweep(
    const Grid& u_previous,
    const Grid& u_current,
    Grid& u_half,
    const SimulationConfig& config,
    TimingInfo& timing
);

bool serial_y_sweep(
    const Grid& u_half,
    const Grid& u_current,
    Grid& u_next,
    const SimulationConfig& config,
    TimingInfo& timing
);

bool solve_wave_serial(
    const SimulationConfig& config,
    Grid& final_result,
    TimingInfo& timing
);
"""

FILES["include/decomposition.hpp"] = """#pragma once

#include "common_types.hpp"

Decomposition create_block_decomposition(
    int global_size,
    int rank,
    int world_size
);
"""

FILES["include/mpi_x_sweep.hpp"] = """#pragma once

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
"""

FILES["include/column_buffer.hpp"] = """#pragma once

#include <vector>

#include "grid.hpp"

void pack_columns(
    const Grid& input,
    int start_column,
    int column_count,
    std::vector<double>& buffer
);

void unpack_columns(
    const std::vector<double>& buffer,
    int start_column,
    int column_count,
    Grid& output
);
"""

FILES["include/mpi_communication.hpp"] = """#pragma once

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
"""

FILES["include/mpi_y_sweep.hpp"] = """#pragma once

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
"""

FILES["include/metrics.hpp"] = """#pragma once

#include <vector>

#include "common_types.hpp"
#include "grid.hpp"

ErrorMetrics compare_grids(
    const Grid& reference,
    const Grid& candidate
);

double calculate_speedup(
    double serial_time,
    double parallel_time
);

double calculate_efficiency(
    double speedup,
    int process_count
);

double calculate_communication_ratio(
    const TimingInfo& timing
);

double calculate_load_imbalance(
    const std::vector<double>& rank_times
);
"""

FILES["include/result_writer.hpp"] = """#pragma once

#include <string>
#include <vector>

#include "common_types.hpp"
#include "grid.hpp"

bool write_grid_csv(
    const Grid& grid,
    const std::string& output_path
);

bool append_benchmark_summary(
    const SimulationConfig& config,
    const TimingInfo& timing,
    const ErrorMetrics& errors,
    int process_count,
    int machine_count,
    double speedup,
    double efficiency,
    const std::string& status,
    const std::string& output_path
);

bool write_rank_timing_csv(
    const std::vector<TimingInfo>& rank_timings,
    const std::vector<std::string>& hostnames,
    const std::vector<Decomposition>& decompositions,
    const std::string& output_path
);
"""

FILES["src/core/grid.cpp"] = """#include "grid.hpp"

#include <algorithm>
#include <stdexcept>

Grid::Grid() = default;

Grid::Grid(int rows, int cols) {
    resize(rows, cols);
}

void Grid::resize(int rows, int cols) {
    if (rows <= 0 || cols <= 0) {
        throw std::invalid_argument("Grid dimensions must be positive");
    }

    rows_ = rows;
    cols_ = cols;
    values_.assign(rows * cols, 0.0);
}

void Grid::fill(double value) {
    std::fill(values_.begin(), values_.end(), value);
}

double& Grid::operator()(int row, int col) {
    return values_.at(row * cols_ + col);
}

const double& Grid::operator()(int row, int col) const {
    return values_.at(row * cols_ + col);
}

double* Grid::data() {
    return values_.data();
}

const double* Grid::data() const {
    return values_.data();
}

int Grid::rows() const {
    return rows_;
}

int Grid::cols() const {
    return cols_;
}

int Grid::size() const {
    return static_cast<int>(values_.size());
}
"""

FILES["src/core/wave_problem.cpp"] = """#include "wave_problem.hpp"

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void apply_boundary_conditions(Grid& grid) {
    const int rows = grid.rows();
    const int cols = grid.cols();

    for (int col = 0; col < cols; ++col) {
        grid(0, col) = 0.0;
        grid(rows - 1, col) = 0.0;
    }

    for (int row = 0; row < rows; ++row) {
        grid(row, 0) = 0.0;
        grid(row, cols - 1) = 0.0;
    }
}

void initialize_wave_problem(
    Grid& u_previous,
    Grid& u_current,
    const SimulationConfig& config
) {
    const int n = config.grid_size;
    const double h = config.domain_length / (n - 1);

    u_previous.resize(n, n);
    u_current.resize(n, n);

    for (int row = 0; row < n; ++row) {
        for (int col = 0; col < n; ++col) {
            const double x = col * h;
            const double y = row * h;
            const double value =
                std::sin(M_PI * x / config.domain_length) *
                std::sin(M_PI * y / config.domain_length);

            u_previous(row, col) = value;
            u_current(row, col) = value;
        }
    }

    apply_boundary_conditions(u_previous);
    apply_boundary_conditions(u_current);
}

double exact_solution(
    double t,
    double x,
    double y,
    const SimulationConfig& config
) {
    const double omega =
        config.wave_speed * M_PI * std::sqrt(2.0) / config.domain_length;

    return std::sin(M_PI * x / config.domain_length) *
           std::sin(M_PI * y / config.domain_length) *
           std::cos(omega * t);
}
"""

FILES["src/solver/thomas.cpp"] = """#include "tridiagonal_solver.hpp"

bool solve_thomas(
    const std::vector<double>& lower,
    const std::vector<double>& diagonal,
    const std::vector<double>& upper,
    const std::vector<double>& rhs,
    std::vector<double>& solution
) {
    (void)lower;
    (void)diagonal;
    (void)upper;
    (void)rhs;
    (void)solution;
    return false;
}

bool solve_tridiagonal(
    const std::string& solver_name,
    const std::vector<double>& lower,
    const std::vector<double>& diagonal,
    const std::vector<double>& upper,
    const std::vector<double>& rhs,
    std::vector<double>& solution
) {
    if (solver_name == "thomas") {
        return solve_thomas(lower, diagonal, upper, rhs, solution);
    }

    if (solver_name == "cr") {
        return solve_cyclic_reduction(lower, diagonal, upper, rhs, solution);
    }

    return false;
}
"""

FILES["src/solver/cyclic_reduction.cpp"] = """#include "tridiagonal_solver.hpp"

bool solve_cyclic_reduction(
    const std::vector<double>& lower,
    const std::vector<double>& diagonal,
    const std::vector<double>& upper,
    const std::vector<double>& rhs,
    std::vector<double>& solution
) {
    (void)lower;
    (void)diagonal;
    (void)upper;
    (void)rhs;
    (void)solution;
    return false;
}
"""

FILES["src/serial/adi_serial.cpp"] = """#include "adi_serial.hpp"

bool serial_x_sweep(
    const Grid& u_previous,
    const Grid& u_current,
    Grid& u_half,
    const SimulationConfig& config,
    TimingInfo& timing
) {
    (void)u_previous;
    (void)u_current;
    (void)u_half;
    (void)config;
    (void)timing;
    return false;
}

bool serial_y_sweep(
    const Grid& u_half,
    const Grid& u_current,
    Grid& u_next,
    const SimulationConfig& config,
    TimingInfo& timing
) {
    (void)u_half;
    (void)u_current;
    (void)u_next;
    (void)config;
    (void)timing;
    return false;
}

bool solve_wave_serial(
    const SimulationConfig& config,
    Grid& final_result,
    TimingInfo& timing
) {
    (void)config;
    (void)final_result;
    (void)timing;
    return false;
}
"""

FILES["src/mpi/decomposition.cpp"] = """#include "decomposition.hpp"

Decomposition create_block_decomposition(
    int global_size,
    int rank,
    int world_size
) {
    Decomposition result;
    result.global_size = global_size;
    result.counts.assign(world_size, 0);
    result.displacements.assign(world_size, 0);

    const int base = global_size / world_size;
    const int remainder = global_size % world_size;

    int offset = 0;
    for (int r = 0; r < world_size; ++r) {
        const int local_count = base + (r < remainder ? 1 : 0);
        result.counts[r] = local_count;
        result.displacements[r] = offset;
        offset += local_count;
    }

    result.local_start = result.displacements[rank];
    result.local_count = result.counts[rank];

    return result;
}
"""

FILES["src/mpi/column_buffer.cpp"] = """#include "column_buffer.hpp"

void pack_columns(
    const Grid& input,
    int start_column,
    int column_count,
    std::vector<double>& buffer
) {
    const int rows = input.rows();
    buffer.assign(rows * column_count, 0.0);

    for (int local_col = 0; local_col < column_count; ++local_col) {
        const int global_col = start_column + local_col;
        for (int row = 0; row < rows; ++row) {
            buffer[local_col * rows + row] = input(row, global_col);
        }
    }
}

void unpack_columns(
    const std::vector<double>& buffer,
    int start_column,
    int column_count,
    Grid& output
) {
    const int rows = output.rows();

    for (int local_col = 0; local_col < column_count; ++local_col) {
        const int global_col = start_column + local_col;
        for (int row = 0; row < rows; ++row) {
            output(row, global_col) = buffer[local_col * rows + row];
        }
    }
}
"""

FILES["src/mpi/communication_nonblocking.cpp"] = """#include "mpi_communication.hpp"

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
"""

FILES["src/mpi/communication_blocking.cpp"] = """#include "mpi_communication.hpp"

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

    MPI_Barrier(communicator);
    const double start = MPI_Wtime();

    if (communication_mode == "nonblocking") {
        const bool ok = gather_distributed_data_nonblocking(
            local_buffer,
            global_buffer,
            counts,
            displacements,
            communicator,
            communication_time
        );
        return ok;
    }

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
"""

FILES["src/mpi/mpi_x_sweep.cpp"] = """#include "mpi_x_sweep.hpp"

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
"""

FILES["src/mpi/mpi_y_sweep.cpp"] = """#include "mpi_y_sweep.hpp"

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
"""

FILES["src/benchmark/metrics.cpp"] = """#include "metrics.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

ErrorMetrics compare_grids(
    const Grid& reference,
    const Grid& candidate
) {
    ErrorMetrics metrics;

    if (reference.rows() != candidate.rows() ||
        reference.cols() != candidate.cols()) {
        return metrics;
    }

    const int n = reference.size();
    double sum_sq = 0.0;

    for (int index = 0; index < n; ++index) {
        const double diff =
            reference.data()[index] - candidate.data()[index];
        const double abs_diff = std::abs(diff);
        metrics.max_absolute_error =
            std::max(metrics.max_absolute_error, abs_diff);
        sum_sq += diff * diff;
    }

    metrics.l2_error = std::sqrt(sum_sq / static_cast<double>(n));
    return metrics;
}

double calculate_speedup(
    double serial_time,
    double parallel_time
) {
    if (parallel_time <= 0.0) {
        return 0.0;
    }
    return serial_time / parallel_time;
}

double calculate_efficiency(
    double speedup,
    int process_count
) {
    if (process_count <= 0) {
        return 0.0;
    }
    return speedup / static_cast<double>(process_count);
}

double calculate_communication_ratio(
    const TimingInfo& timing
) {
    if (timing.total <= 0.0) {
        return 0.0;
    }
    return (timing.communication_x + timing.communication_y) / timing.total;
}

double calculate_load_imbalance(
    const std::vector<double>& rank_times
) {
    if (rank_times.empty()) {
        return 0.0;
    }

    const double sum =
        std::accumulate(rank_times.begin(), rank_times.end(), 0.0);
    const double average = sum / static_cast<double>(rank_times.size());
    const double max_time =
        *std::max_element(rank_times.begin(), rank_times.end());

    if (average <= 0.0) {
        return 0.0;
    }

    return (max_time - average) / average;
}
"""

FILES["src/benchmark/result_writer.cpp"] = """#include "result_writer.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>

namespace {

bool file_exists(const std::string& path) {
    std::ifstream input(path);
    return input.good();
}

std::string current_timestamp_placeholder() {
    return "0";
}

}  // namespace

bool write_grid_csv(
    const Grid& grid,
    const std::string& output_path
) {
    std::ofstream output(output_path);
    if (!output) {
        return false;
    }

    output << "row,column,value\\n";
    output << std::setprecision(12) << std::fixed;

    for (int row = 0; row < grid.rows(); ++row) {
        for (int col = 0; col < grid.cols(); ++col) {
            output << row << ',' << col << ',' << grid(row, col) << '\\n';
        }
    }

    return true;
}

bool append_benchmark_summary(
    const SimulationConfig& config,
    const TimingInfo& timing,
    const ErrorMetrics& errors,
    int process_count,
    int machine_count,
    double speedup,
    double efficiency,
    const std::string& status,
    const std::string& output_path
) {
    const bool needs_header = !file_exists(output_path);
    std::ofstream output(output_path, std::ios::app);
    if (!output) {
        return false;
    }

    if (needs_header) {
        output << "run_id,timestamp,grid_size,time_steps,processes,machines,"
                  "solver,communication,total_time,compute_x,communication_x,"
                  "compute_y,communication_y,waiting_time,max_error,l2_error,"
                  "speedup,efficiency,status\\n";
    }

    output << "0," << current_timestamp_placeholder() << ','
           << config.grid_size << ',' << config.time_steps << ','
           << process_count << ',' << machine_count << ','
           << config.solver << ',' << config.communication << ','
           << timing.total << ',' << timing.compute_x << ','
           << timing.communication_x << ',' << timing.compute_y << ','
           << timing.communication_y << ',' << timing.waiting << ','
           << errors.max_absolute_error << ',' << errors.l2_error << ','
           << speedup << ',' << efficiency << ',' << status << '\\n';

    return true;
}

bool write_rank_timing_csv(
    const std::vector<TimingInfo>& rank_timings,
    const std::vector<std::string>& hostnames,
    const std::vector<Decomposition>& decompositions,
    const std::string& output_path
) {
    std::ofstream output(output_path);
    if (!output) {
        return false;
    }

    output << "run_id,rank,hostname,local_start,local_count,compute_x,"
              "communication_x,compute_y,communication_y,total_time\\n";

    for (std::size_t rank = 0; rank < rank_timings.size(); ++rank) {
        const TimingInfo& timing = rank_timings[rank];
        const std::string hostname =
            rank < hostnames.size() ? hostnames[rank] : "unknown";
        const Decomposition& decomposition =
            rank < decompositions.size()
                ? decompositions[rank]
                : Decomposition{};

        output << "0," << rank << ',' << hostname << ','
               << decomposition.local_start << ','
               << decomposition.local_count << ','
               << timing.compute_x << ',' << timing.communication_x << ','
               << timing.compute_y << ',' << timing.communication_y << ','
               << timing.total << '\\n';
    }

    return true;
}
"""

FILES["src/main_serial.cpp"] = """#include <iostream>

#include "adi_serial.hpp"
#include "common_types.hpp"
#include "grid.hpp"
#include "result_writer.hpp"

int main(int argc, char** argv) {
    SimulationConfig config;

    (void)argc;
    (void)argv;

    Grid final_result;
    TimingInfo timing;

    const bool success = solve_wave_serial(
        config,
        final_result,
        timing
    );

    if (!success) {
        std::cerr << "status=failed\\n";
        return 1;
    }

    if (!config.output_path.empty()) {
        write_grid_csv(final_result, config.output_path);
    }

    std::cout << "mode=serial\\n";
    std::cout << "solver=" << config.solver << "\\n";
    std::cout << "grid_size=" << config.grid_size << "\\n";
    std::cout << "time_steps=" << config.time_steps << "\\n";
    std::cout << "status=success\\n";
    std::cout << "runtime_seconds=" << timing.total << "\\n";

    return 0;
}
"""

FILES["src/main_mpi.cpp"] = """#include <mpi.h>

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
        std::cout << "mode=mpi\\n";
        std::cout << "grid_size=" << config.grid_size << "\\n";
        std::cout << "time_steps=" << config.time_steps << "\\n";
        std::cout << "processes=" << world_size << "\\n";
        std::cout << "solver=" << config.solver << "\\n";
        std::cout << "communication="
                  << config.communication << "\\n";
        std::cout << "status="
                  << (success ? "success" : "failed")
                  << "\\n";
        std::cout << "total_seconds="
                  << global_total << "\\n";
    }

    MPI_Finalize();
    return success ? 0 : 1;
}
"""

def test_stub(name, extra_mpi=False):
    mpi_init = """
    MPI_Init(nullptr, nullptr);
""" if extra_mpi else ""
    mpi_finalize = """
    MPI_Finalize();
""" if extra_mpi else ""
    return f"""#include <iostream>

{"#include <mpi.h>" if extra_mpi else ""}

int main() {{
{mpi_init}
    std::cout << "{name}: pending implementation\\n";
{mpi_finalize}
    return 0;
}}
"""

FILES["tests/test_thomas.cpp"] = test_stub("test_thomas")
FILES["tests/test_cyclic_reduction.cpp"] = test_stub("test_cyclic_reduction")

FILES["tests/test_decomposition.cpp"] = """#include <iostream>

#include "decomposition.hpp"

int main() {
    const int cases[][2] = {{10, 3}, {11, 4}, {4, 8}};
    bool ok = true;

    for (const auto& test_case : cases) {
        const int n = test_case[0];
        const int p = test_case[1];

        int total = 0;
        int previous_end = 0;

        for (int rank = 0; rank < p; ++rank) {
            const Decomposition d = create_block_decomposition(n, rank, p);
            total += d.local_count;

            if (d.local_start != previous_end) {
                ok = false;
            }

            previous_end = d.local_start + d.local_count;
        }

        if (total != n) {
            ok = false;
        }
    }

    if (!ok) {
        std::cerr << "test_decomposition: failed\\n";
        return 1;
    }

    std::cout << "test_decomposition: passed\\n";
    return 0;
}
"""

FILES["tests/test_column_buffer.cpp"] = """#include <cmath>
#include <iostream>

#include "column_buffer.hpp"
#include "grid.hpp"

int main() {
    Grid grid(3, 3);
    grid(0, 0) = 1.0;
    grid(0, 1) = 2.0;
    grid(0, 2) = 3.0;
    grid(1, 0) = 4.0;
    grid(1, 1) = 5.0;
    grid(1, 2) = 6.0;
    grid(2, 0) = 7.0;
    grid(2, 1) = 8.0;
    grid(2, 2) = 9.0;

    std::vector<double> buffer;
    pack_columns(grid, 1, 1, buffer);

    if (buffer.size() != 3 ||
        std::abs(buffer[0] - 2.0) > 1e-12 ||
        std::abs(buffer[1] - 5.0) > 1e-12 ||
        std::abs(buffer[2] - 8.0) > 1e-12) {
        std::cerr << "test_column_buffer: pack failed\\n";
        return 1;
    }

    Grid restored(3, 3);
    restored.fill(0.0);
    unpack_columns(buffer, 1, 1, restored);

    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            if (std::abs(grid(row, col) - restored(row, col)) > 1e-12) {
                std::cerr << "test_column_buffer: unpack failed\\n";
                return 1;
            }
        }
    }

    std::cout << "test_column_buffer: passed\\n";
    return 0;
}
"""

FILES["tests/test_metrics.cpp"] = """#include <cmath>
#include <iostream>

#include "grid.hpp"
#include "metrics.hpp"

int main() {
    Grid reference(2, 2);
    Grid candidate(2, 2);

    reference(0, 0) = 1.0;
    reference(0, 1) = 2.0;
    reference(1, 0) = 3.0;
    reference(1, 1) = 4.0;

    candidate = reference;
    candidate(1, 1) += 0.1;

    const ErrorMetrics errors = compare_grids(reference, candidate);
    if (errors.max_absolute_error < 0.09) {
        std::cerr << "test_metrics: compare_grids failed\\n";
        return 1;
    }

    const double speedup = calculate_speedup(10.0, 5.0);
    const double efficiency = calculate_efficiency(speedup, 4);

    if (std::abs(speedup - 2.0) > 1e-12 || std::abs(efficiency - 0.5) > 1e-12) {
        std::cerr << "test_metrics: speedup/efficiency failed\\n";
        return 1;
    }

    std::cout << "test_metrics: passed\\n";
    return 0;
}
"""

FILES["tests/test_mpi_x_sweep.cpp"] = test_stub("test_mpi_x_sweep", extra_mpi=True)
FILES["tests/test_mpi_y_sweep.cpp"] = test_stub("test_mpi_y_sweep", extra_mpi=True)

FILES["scripts/sync_cluster.sh"] = """#!/usr/bin/env bash

set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

rsync -az \\
    --delete \\
    --exclude build \\
    --exclude .git \\
    "$PROJECT_DIR/" \\
    slave1:~/wave-mpi/

rsync -az \\
    --delete \\
    --exclude build \\
    --exclude .git \\
    "$PROJECT_DIR/" \\
    slave2:~/wave-mpi/

echo "Cluster synchronization completed."
"""

FILES["scripts/run_experiments.py"] = """#!/usr/bin/env python3
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD_DIR = ROOT / "build"

GRID_SIZES = [127, 255, 511, 1023]
PROCESS_COUNTS = [1, 2, 4, 8]
COMMUNICATION_MODES = ["blocking", "nonblocking"]
REPEAT_COUNT = 5


def main() -> None:
    print("run_experiments.py: pending full benchmark automation")
    print(f"project_root={ROOT}")
    print(f"planned_grid_sizes={GRID_SIZES}")
    print(f"planned_process_counts={PROCESS_COUNTS}")
    print(f"planned_modes={COMMUNICATION_MODES}")
    print(f"repeat_count={REPEAT_COUNT}")


if __name__ == "__main__":
    main()
"""

FILES["scripts/plot_runtime.py"] = """#!/usr/bin/env python3
from pathlib import Path

SUMMARY_PATH = Path(__file__).resolve().parent.parent / "results" / "summaries" / "benchmark_summary.csv"
OUTPUT_PATH = Path(__file__).resolve().parent.parent / "results" / "figures" / "runtime.png"


def main() -> None:
    print(f"plot_runtime.py: read {SUMMARY_PATH}, write {OUTPUT_PATH}")


if __name__ == "__main__":
    main()
"""

FILES["scripts/plot_speedup.py"] = """#!/usr/bin/env python3
from pathlib import Path

SUMMARY_PATH = Path(__file__).resolve().parent.parent / "results" / "summaries" / "benchmark_summary.csv"
OUTPUT_PATH = Path(__file__).resolve().parent.parent / "results" / "figures" / "speedup.png"


def main() -> None:
    print(f"plot_speedup.py: read {SUMMARY_PATH}, write {OUTPUT_PATH}")


if __name__ == "__main__":
    main()
"""

FILES["scripts/plot_load_balance.py"] = """#!/usr/bin/env python3
from pathlib import Path

TIMING_DIR = Path(__file__).resolve().parent.parent / "results" / "summaries"
OUTPUT_PATH = Path(__file__).resolve().parent.parent / "results" / "figures" / "load_balance.png"


def main() -> None:
    print(f"plot_load_balance.py: read {TIMING_DIR}, write {OUTPUT_PATH}")


if __name__ == "__main__":
    main()
"""

FILES["scripts/calibrate_size.py"] = """#!/usr/bin/env python3

def main() -> None:
    print("calibrate_size.py: pending grid size calibration helper")


if __name__ == "__main__":
    main()
"""


def main() -> None:
    for directory in DIRS:
        (ROOT / directory).mkdir(parents=True, exist_ok=True)

    for relative_path, content in FILES.items():
        target = ROOT / relative_path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(content, encoding="utf-8", newline="\n")

    for keep_dir in ("results/raw", "results/summaries", "results/figures"):
        keep_file = ROOT / keep_dir / ".gitkeep"
        keep_file.write_text("", encoding="utf-8")

    print(f"Created wave-mpi project at {ROOT}")
    print(f"Files written: {len(FILES)}")


if __name__ == "__main__":
    main()
