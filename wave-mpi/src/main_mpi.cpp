#include <mpi.h>

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
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

namespace {

void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name
              << " --grid-size N --time-steps N [options]\n"
              << "Options:\n"
              << "  --grid-size N         Grid dimension (default: 255)\n"
              << "  --time-steps N        Number of time steps (default: 50)\n"
              << "  --domain-length L     Domain length (default: 1.0)\n"
              << "  --total-time T        Total simulation time (default: 1.0)\n"
              << "  --wave-speed C        Wave speed (default: 1.0)\n"
              << "  --solver NAME         Solver: thomas or cr (default: thomas)\n"
              << "  --communication MODE  Communication: blocking or nonblocking (default: blocking)\n"
              << "  --reference PATH      Path to serial reference CSV\n"
              << "  --output PATH         Output CSV file path for final grid\n"
              << "  --machine-count N     Number of physical machines (default: 1)\n";
}

bool parse_serial_args(int argc, char** argv, SimulationConfig& config) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return false;
        }

        if (i + 1 >= argc) {
            std::cerr << "Missing value for " << arg << '\n';
            return false;
        }

        const std::string value = argv[++i];

        if (arg == "--grid-size") {
            config.grid_size = std::stoi(value);
        } else if (arg == "--time-steps") {
            config.time_steps = std::stoi(value);
        } else if (arg == "--domain-length") {
            config.domain_length = std::stod(value);
        } else if (arg == "--total-time") {
            config.total_time = std::stod(value);
        } else if (arg == "--wave-speed") {
            config.wave_speed = std::stod(value);
        } else if (arg == "--solver") {
            config.solver = value;
        } else if (arg == "--communication") {
            config.communication = value;
        } else if (arg == "--reference") {
            config.reference_path = value;
        } else if (arg == "--output") {
            config.output_path = value;
        } else {
            std::cerr << "Unknown option: " << arg << '\n';
            print_usage(argv[0]);
            return false;
        }
    }

    if (config.grid_size <= 0) {
        std::cerr << "grid_size must be positive\n";
        return false;
    }
    if (config.time_steps <= 0) {
        std::cerr << "time_steps must be positive\n";
        return false;
    }
    if (config.solver != "thomas" && config.solver != "cr") {
        std::cerr << "solver must be thomas or cr\n";
        return false;
    }
    if (config.communication != "blocking" && config.communication != "nonblocking") {
        std::cerr << "communication must be blocking or nonblocking\n";
        return false;
    }

    return true;
}

void broadcast_config(SimulationConfig& config) {
    MPI_Bcast(&config.grid_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&config.time_steps, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&config.domain_length, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(&config.total_time, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(&config.wave_speed, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    int solver_len = static_cast<int>(config.solver.size());
    MPI_Bcast(&solver_len, 1, MPI_INT, 0, MPI_COMM_WORLD);
    config.solver.resize(solver_len);
    MPI_Bcast(&config.solver[0], solver_len, MPI_CHAR, 0, MPI_COMM_WORLD);

    int comm_len = static_cast<int>(config.communication.size());
    MPI_Bcast(&comm_len, 1, MPI_INT, 0, MPI_COMM_WORLD);
    config.communication.resize(comm_len);
    MPI_Bcast(&config.communication[0], comm_len, MPI_CHAR, 0, MPI_COMM_WORLD);

    int output_len = static_cast<int>(config.output_path.size());
    MPI_Bcast(&output_len, 1, MPI_INT, 0, MPI_COMM_WORLD);
    config.output_path.resize(output_len);
    MPI_Bcast(&config.output_path[0], output_len, MPI_CHAR, 0, MPI_COMM_WORLD);

    int ref_len = static_cast<int>(config.reference_path.size());
    MPI_Bcast(&ref_len, 1, MPI_INT, 0, MPI_COMM_WORLD);
    config.reference_path.resize(ref_len);
    MPI_Bcast(&config.reference_path[0], ref_len, MPI_CHAR, 0, MPI_COMM_WORLD);
}

std::string get_hostname() {
    char name[MPI_MAX_PROCESSOR_NAME];
    int len = 0;
    MPI_Get_processor_name(name, &len);
    name[len] = '\0';
    return {name};
}

Grid read_reference_grid(const std::string& path) {
    Grid grid;
    if (path.empty()) return grid;

    std::ifstream input(path);
    if (!input) return grid;

    std::string header;
    std::getline(input, header);

    int max_row = -1;
    int max_col = -1;

    std::string line;
    while (std::getline(input, line)) {
        std::istringstream ss(line);
        std::string token;
        int row = -1;
        int col = -1;

        if (std::getline(ss, token, ',')) row = std::stoi(token);
        if (std::getline(ss, token, ',')) col = std::stoi(token);

        if (row > max_row) max_row = row;
        if (col > max_col) max_col = col;
    }

    const int rows = max_row + 1;
    const int cols = max_col + 1;
    grid.resize(rows, cols);
    grid.fill(0.0);

    input.clear();
    input.seekg(0);
    std::getline(input, header);

    while (std::getline(input, line)) {
        std::istringstream ss(line);
        std::string token;
        int row = -1;
        int col = -1;
        double value = 0.0;

        if (std::getline(ss, token, ',')) row = std::stoi(token);
        if (std::getline(ss, token, ',')) col = std::stoi(token);
        if (std::getline(ss, token, ',')) value = std::stod(token);

        if (row >= 0 && row < rows && col >= 0 && col < cols) {
            grid(row, col) = value;
        }
    }

    return grid;
}

}  // namespace

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = 0;
    int world_size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    SimulationConfig config;
    bool cli_ok = true;

    if (rank == 0) {
        cli_ok = parse_serial_args(argc, argv, config);
    }

    int cli_result = cli_ok ? 1 : 0;
    MPI_Bcast(&cli_result, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (cli_result == 0) {
        MPI_Finalize();
        return 1;
    }

    broadcast_config(config);

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

    u_half.resize(config.grid_size, config.grid_size);
    u_next.resize(config.grid_size, config.grid_size);

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

        if (!success) break;

        success = mpi_y_sweep(
            u_half,
            u_current,
            u_next,
            config,
            decomposition,
            MPI_COMM_WORLD,
            local_timing
        );

        if (!success) break;

        u_previous = u_current;
        u_current = u_next;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    local_timing.total = MPI_Wtime() - total_start;

    // Gather all rank timing info to rank 0 for reporting
    const std::string hostname = get_hostname();

    int hostname_len = static_cast<int>(hostname.size());
    std::vector<int> hostname_lengths(world_size);
    std::vector<int> hostname_displs(world_size);

    // Gather hostname lengths
    MPI_Gather(
        &hostname_len, 1, MPI_INT,
        hostname_lengths.data(), 1, MPI_INT,
        0, MPI_COMM_WORLD
    );

    int total_hostname_chars = 0;
    std::vector<char> all_hostnames;
    if (rank == 0) {
        hostname_displs[0] = 0;
        for (int i = 0; i < world_size; ++i) {
            if (i > 0) hostname_displs[i] = hostname_displs[i - 1] + hostname_lengths[i - 1];
            total_hostname_chars += hostname_lengths[i];
        }
        all_hostnames.resize(total_hostname_chars);
    }

    MPI_Gatherv(
        hostname.data(), hostname_len, MPI_CHAR,
        all_hostnames.data(), hostname_lengths.data(), hostname_displs.data(), MPI_CHAR,
        0, MPI_COMM_WORLD
    );

    // Gather timing data
    std::vector<double> all_total_times(world_size);
    MPI_Gather(
        &local_timing.total, 1, MPI_DOUBLE,
        all_total_times.data(), 1, MPI_DOUBLE,
        0, MPI_COMM_WORLD
    );

    std::vector<double> all_compute_x(world_size);
    MPI_Gather(
        &local_timing.compute_x, 1, MPI_DOUBLE,
        all_compute_x.data(), 1, MPI_DOUBLE,
        0, MPI_COMM_WORLD
    );

    std::vector<double> all_communication_x(world_size);
    MPI_Gather(
        &local_timing.communication_x, 1, MPI_DOUBLE,
        all_communication_x.data(), 1, MPI_DOUBLE,
        0, MPI_COMM_WORLD
    );

    std::vector<double> all_compute_y(world_size);
    MPI_Gather(
        &local_timing.compute_y, 1, MPI_DOUBLE,
        all_compute_y.data(), 1, MPI_DOUBLE,
        0, MPI_COMM_WORLD
    );

    std::vector<double> all_communication_y(world_size);
    MPI_Gather(
        &local_timing.communication_y, 1, MPI_DOUBLE,
        all_communication_y.data(), 1, MPI_DOUBLE,
        0, MPI_COMM_WORLD
    );

    // Gather decompositions
    std::vector<int> all_local_starts(world_size);
    std::vector<int> all_local_counts(world_size);
    int local_start = decomposition.local_start;
    int local_count = decomposition.local_count;

    MPI_Gather(
        &local_start, 1, MPI_INT,
        all_local_starts.data(), 1, MPI_INT,
        0, MPI_COMM_WORLD
    );
    MPI_Gather(
        &local_count, 1, MPI_INT,
        all_local_counts.data(), 1, MPI_INT,
        0, MPI_COMM_WORLD
    );

    // Reduce compute/communication timings to max for global report
    double global_total = 0.0;
    double global_compute_x = 0.0;
    double global_communication_x = 0.0;
    double global_compute_y = 0.0;
    double global_communication_y = 0.0;

    MPI_Reduce(&local_timing.total, &global_total, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_timing.compute_x, &global_compute_x, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_timing.communication_x, &global_communication_x, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_timing.compute_y, &global_compute_y, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_timing.communication_y, &global_communication_y, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    // Reduce error info to rank 0
    int global_success = success ? 1 : 0;
    int all_success = 0;
    MPI_Allreduce(&global_success, &all_success, 1, MPI_INT, MPI_LAND, MPI_COMM_WORLD);
    success = (all_success != 0);

    if (rank == 0) {
        // Compute metrics
        int machine_count = 1;  // default, could be parsed from CLI

        double max_absolute_error = 0.0;
        double l2_error = 0.0;
        std::string correctness_status = "not_checked";

        if (!config.reference_path.empty()) {
            Grid reference = read_reference_grid(config.reference_path);
            if (reference.rows() > 0 && reference.cols() > 0) {
                ErrorMetrics errors = compare_grids(reference, u_next);
                max_absolute_error = errors.max_absolute_error;
                l2_error = errors.l2_error;
                correctness_status = (max_absolute_error < 1e-8) ? "passed" : "failed";
            }
        }

        // Console output: part 1 – summary
        std::cout << "mode=mpi\n";
        std::cout << "grid_size=" << config.grid_size << "\n";
        std::cout << "time_steps=" << config.time_steps << "\n";
        std::cout << "processes=" << world_size << "\n";
        std::cout << "solver=" << config.solver << "\n";
        std::cout << "communication=" << config.communication << "\n";
        std::cout << "status=" << (success ? "success" : "failed") << "\n\n";

        std::cout << "total_seconds=" << global_total << "\n";
        std::cout << "compute_x_seconds=" << global_compute_x << "\n";
        std::cout << "communication_x_seconds=" << global_communication_x << "\n";
        std::cout << "compute_y_seconds=" << global_compute_y << "\n";
        std::cout << "communication_y_seconds=" << global_communication_y << "\n";
        std::cout << "waiting_seconds=" << local_timing.waiting << "\n\n";

        if (!config.reference_path.empty()) {
            std::cout << "max_absolute_error=" << max_absolute_error << "\n";
            std::cout << "l2_error=" << l2_error << "\n";
            std::cout << "correctness=" << correctness_status << "\n";
        }

        // Write output grid if requested
        if (!config.output_path.empty()) {
            write_grid_csv(u_next, config.output_path);
        }

        // Write per-rank timing
        std::vector<TimingInfo> rank_timings(world_size);
        std::vector<std::string> hostnames(world_size);
        std::vector<Decomposition> decompositions(world_size);

        for (int i = 0; i < world_size; ++i) {
            rank_timings[i].total = all_total_times[i];
            rank_timings[i].compute_x = all_compute_x[i];
            rank_timings[i].communication_x = all_communication_x[i];
            rank_timings[i].compute_y = all_compute_y[i];
            rank_timings[i].communication_y = all_communication_y[i];

            hostnames[i] = std::string(
                &all_hostnames[hostname_displs[i]],
                hostname_lengths[i]
            );

            decompositions[i].global_size = config.grid_size;
            decompositions[i].local_start = all_local_starts[i];
            decompositions[i].local_count = all_local_counts[i];
        }

        std::ostringstream rank_path;
        rank_path << "results/summaries/rank_timing_N"
                  << config.grid_size << "_P" << world_size << ".csv";
        write_rank_timing_csv(rank_timings, hostnames, decompositions, rank_path.str());

        // Write benchmark summary line
        double speedup = 0.0;
        double efficiency = 0.0;
        // speedup and efficiency require serial time which we may not have here;
        // the benchmark script will compute these.  Write raw data regardless.

        TimingInfo max_timing;
        max_timing.total = global_total;
        max_timing.compute_x = global_compute_x;
        max_timing.communication_x = global_communication_x;
        max_timing.compute_y = global_compute_y;
        max_timing.communication_y = global_communication_y;
        max_timing.waiting = local_timing.waiting;

        ErrorMetrics errors_for_csv;
        errors_for_csv.max_absolute_error = max_absolute_error;
        errors_for_csv.l2_error = l2_error;

        append_benchmark_summary(
            config,
            max_timing,
            errors_for_csv,
            world_size,
            machine_count,
            speedup,
            efficiency,
            success ? "success" : "failed",
            "results/summaries/benchmark_summary.csv"
        );
    }

    MPI_Finalize();
    return success ? 0 : 1;
}