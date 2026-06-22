#include <iostream>

#include "decomposition.hpp"

int main() {
    const int cases[][2] = {{10, 3}, {11, 4}, {4, 8}};
    bool ok = true;

    for (const auto& test_case : cases) {
        const int n = test_case[0];
        const int p = test_case[1];

        int total = 0;
        int previous_end = 0;

        for (int rank = 0; rank < p; ++rank) {
            const Decomposition d = create_block_decomposition(n, rank, p);
            total += d.local_count;

            if (d.local_start != previous_end) {
                ok = false;
            }

            previous_end = d.local_start + d.local_count;
        }

        if (total != n) {
            ok = false;
        }
    }

    if (!ok) {
        std::cerr << "test_decomposition: failed\n";
        return 1;
    }

    std::cout << "test_decomposition: passed\n";
    return 0;
}
