#pragma once

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
