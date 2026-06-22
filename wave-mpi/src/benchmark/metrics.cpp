#include "metrics.hpp"

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
