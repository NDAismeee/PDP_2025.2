#pragma once

#include "common_types.hpp"

Decomposition create_block_decomposition(
    int global_size,
    int rank,
    int world_size
);
