#pragma once

#include <vector>

#include "grid.hpp"

void pack_columns(
    const Grid& input,
    int start_column,
    int column_count,
    std::vector<double>& buffer
);

void unpack_columns(
    const std::vector<double>& buffer,
    int start_column,
    int column_count,
    Grid& output
);
