#pragma once

#include <vector>

class Grid {
public:
    Grid();
    Grid(int rows, int cols);

    void resize(int rows, int cols);
    void fill(double value);

    double& operator()(int row, int col);
    const double& operator()(int row, int col) const;

    double* data();
    const double* data() const;

    int rows() const;
    int cols() const;
    int size() const;

private:
    int rows_ = 0;
    int cols_ = 0;
    std::vector<double> values_;
};
