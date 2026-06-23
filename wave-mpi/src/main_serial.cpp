#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "adi_serial.hpp"
#include "common_types.hpp"
#include "grid.hpp"
#include "result_writer.hpp"

namespace {

void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name
              << " --grid-size N --time-steps N [options]\n"
              << "Options:\n"
              << "  --grid-size N        Grid dimension (default: 255)\n"
              << "  --time-steps N       Number of time steps (default: 50)\n"
              << "  --domain-length L    Domain length (default: 1.0)\n"
              << "  --total-time T       Total simulation time (default: 1.0)\n"
              << "  --wave-speed C       Wave speed (default: 1.0)\n"
              << "  --solver NAME        Solver: thomas or cr (default: thomas)\n"
              << "  --output PATH        Output CSV file path\n";
}

bool parse_args(int argc, char** argv, SimulationConfig& config) {
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

    return true;
}

}  // namespace

int main(int argc, char** argv) {
    SimulationConfig config;

    if (!parse_args(argc, argv, config)) {
        return 1;
    }

    Grid final_result;
    TimingInfo timing;

    const bool success = solve_wave_serial(
        config,
        final_result,
        timing
    );

    if (!success) {
        std::cerr << "status=failed\n";
        return 1;
    }

    if (!config.output_path.empty()) {
        write_grid_csv(final_result, config.output_path);
    }

    std::cout << "mode=serial\n";
    std::cout << "solver=" << config.solver << "\n";
    std::cout << "grid_size=" << config.grid_size << "\n";
    std::cout << "time_steps=" << config.time_steps << "\n";
    std::cout << "status=success\n";
    std::cout << "runtime_seconds=" << timing.total << "\n";
    if (!config.output_path.empty()) {
        std::cout << "output=" << config.output_path << "\n";
    }

    return 0;
}