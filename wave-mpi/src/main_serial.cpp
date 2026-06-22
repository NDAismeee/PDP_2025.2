#include <iostream>

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

    return 0;
}
