#pragma once

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
