#include <cmath>
#include <iostream>

#include "grid.hpp"
#include "metrics.hpp"

int main() {
    Grid reference(2, 2);
    Grid candidate(2, 2);

    reference(0, 0) = 1.0;
    reference(0, 1) = 2.0;
    reference(1, 0) = 3.0;
    reference(1, 1) = 4.0;

    candidate = reference;
    candidate(1, 1) += 0.1;

    const ErrorMetrics errors = compare_grids(reference, candidate);
    if (errors.max_absolute_error < 0.09) {
        std::cerr << "test_metrics: compare_grids failed\n";
        return 1;
    }

    const double speedup = calculate_speedup(10.0, 5.0);
    const double efficiency = calculate_efficiency(speedup, 4);

    if (std::abs(speedup - 2.0) > 1e-12 || std::abs(efficiency - 0.5) > 1e-12) {
        std::cerr << "test_metrics: speedup/efficiency failed\n";
        return 1;
    }

    std::cout << "test_metrics: passed\n";
    return 0;
}
