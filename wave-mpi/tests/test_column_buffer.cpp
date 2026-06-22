#include <cmath>
#include <iostream>

#include "column_buffer.hpp"
#include "grid.hpp"

int main() {
    Grid grid(3, 3);
    grid(0, 0) = 1.0;
    grid(0, 1) = 2.0;
    grid(0, 2) = 3.0;
    grid(1, 0) = 4.0;
    grid(1, 1) = 5.0;
    grid(1, 2) = 6.0;
    grid(2, 0) = 7.0;
    grid(2, 1) = 8.0;
    grid(2, 2) = 9.0;

    std::vector<double> buffer;
    pack_columns(grid, 1, 1, buffer);

    if (buffer.size() != 3 ||
        std::abs(buffer[0] - 2.0) > 1e-12 ||
        std::abs(buffer[1] - 5.0) > 1e-12 ||
        std::abs(buffer[2] - 8.0) > 1e-12) {
        std::cerr << "test_column_buffer: pack failed\n";
        return 1;
    }

    Grid restored(3, 3);
    restored.fill(0.0);
    unpack_columns(buffer, 1, 1, restored);

    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            if (std::abs(grid(row, col) - restored(row, col)) > 1e-12) {
                std::cerr << "test_column_buffer: unpack failed\n";
                return 1;
            }
        }
    }

    std::cout << "test_column_buffer: passed\n";
    return 0;
}
