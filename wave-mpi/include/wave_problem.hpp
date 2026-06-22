#pragma once

#include "common_types.hpp"
#include "grid.hpp"

void initialize_wave_problem(
    Grid& u_previous,
    Grid& u_current,
    const SimulationConfig& config
);

void apply_boundary_conditions(Grid& grid);

double exact_solution(
    double t,
    double x,
    double y,
    const SimulationConfig& config
);
