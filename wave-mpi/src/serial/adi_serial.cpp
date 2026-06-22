#include "adi_serial.hpp"

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
