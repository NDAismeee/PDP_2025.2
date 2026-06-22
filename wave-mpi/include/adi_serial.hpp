#pragma once

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
