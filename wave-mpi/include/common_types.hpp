#pragma once

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
