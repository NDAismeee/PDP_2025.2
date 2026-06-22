#include "grid.hpp"

#include <algorithm>
#include <stdexcept>

Grid::Grid() = default;

Grid::Grid(int rows, int cols) {
    resize(rows, cols);
}

void Grid::resize(int rows, int cols) {
    if (rows <= 0 || cols <= 0) {
        throw std::invalid_argument("Grid dimensions must be positive");
    }

    rows_ = rows;
    cols_ = cols;
    values_.assign(rows * cols, 0.0);
}

void Grid::fill(double value) {
    std::fill(values_.begin(), values_.end(), value);
}

double& Grid::operator()(int row, int col) {
    return values_.at(row * cols_ + col);
}

const double& Grid::operator()(int row, int col) const {
    return values_.at(row * cols_ + col);
}

double* Grid::data() {
    return values_.data();
}

const double* Grid::data() const {
    return values_.data();
}

int Grid::rows() const {
    return rows_;
}

int Grid::cols() const {
    return cols_;
}

int Grid::size() const {
    return static_cast<int>(values_.size());
}
