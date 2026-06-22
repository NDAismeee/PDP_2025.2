#include "column_buffer.hpp"

void pack_columns(
    const Grid& input,
    int start_column,
    int column_count,
    std::vector<double>& buffer
) {
    const int rows = input.rows();
    buffer.assign(rows * column_count, 0.0);

    for (int local_col = 0; local_col < column_count; ++local_col) {
        const int global_col = start_column + local_col;
        for (int row = 0; row < rows; ++row) {
            buffer[local_col * rows + row] = input(row, global_col);
        }
    }
}

void unpack_columns(
    const std::vector<double>& buffer,
    int start_column,
    int column_count,
    Grid& output
) {
    const int rows = output.rows();

    for (int local_col = 0; local_col < column_count; ++local_col) {
        const int global_col = start_column + local_col;
        for (int row = 0; row < rows; ++row) {
            output(row, global_col) = buffer[local_col * rows + row];
        }
    }
}
