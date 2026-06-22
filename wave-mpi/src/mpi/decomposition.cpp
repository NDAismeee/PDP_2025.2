#include "decomposition.hpp"

Decomposition create_block_decomposition(
    int global_size,
    int rank,
    int world_size
) {
    Decomposition result;
    result.global_size = global_size;
    result.counts.assign(world_size, 0);
    result.displacements.assign(world_size, 0);

    const int base = global_size / world_size;
    const int remainder = global_size % world_size;

    int offset = 0;
    for (int r = 0; r < world_size; ++r) {
        const int local_count = base + (r < remainder ? 1 : 0);
        result.counts[r] = local_count;
        result.displacements[r] = offset;
        offset += local_count;
    }

    result.local_start = result.displacements[rank];
    result.local_count = result.counts[rank];

    return result;
}
