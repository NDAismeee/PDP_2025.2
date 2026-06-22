#include "result_writer.hpp"

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

    output << "row,column,value\n";
    output << std::setprecision(12) << std::fixed;

    for (int row = 0; row < grid.rows(); ++row) {
        for (int col = 0; col < grid.cols(); ++col) {
            output << row << ',' << col << ',' << grid(row, col) << '\n';
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
                  "speedup,efficiency,status\n";
    }

    output << "0," << current_timestamp_placeholder() << ','
           << config.grid_size << ',' << config.time_steps << ','
           << process_count << ',' << machine_count << ','
           << config.solver << ',' << config.communication << ','
           << timing.total << ',' << timing.compute_x << ','
           << timing.communication_x << ',' << timing.compute_y << ','
           << timing.communication_y << ',' << timing.waiting << ','
           << errors.max_absolute_error << ',' << errors.l2_error << ','
           << speedup << ',' << efficiency << ',' << status << '\n';

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
              "communication_x,compute_y,communication_y,total_time\n";

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
               << timing.total << '\n';
    }

    return true;
}
