#include "mpi_x_sweep.hpp"
#include "tridiagonal_solver.hpp"
#include "wave_problem.hpp"
#include <mpi.h>
#include <vector>

namespace {

double compute_mu(const SimulationConfig& config)
{
    const int n = config.grid_size;
    const double h = config.domain_length / static_cast<double>(n - 1);
    const double dt = config.total_time / static_cast<double>(config.time_steps);
    const double ratio = config.wave_speed * dt / h;
    return 0.5 * ratio * ratio;
}

} // namespace

bool mpi_x_sweep(
    const Grid& u_previous,
    const Grid& u_current,
    Grid& u_half,
    const SimulationConfig& config,
    const Decomposition& decomposition,
    MPI_Comm communicator,
    TimingInfo& timing
)
{
    int rank;
    int world_size;
    MPI_Comm_rank(communicator, &rank);
    MPI_Comm_size(communicator, &world_size);

    const int n = config.grid_size;
    u_half.resize(n, n);
    u_half.fill(0.0);

    // --- BƯỚC 1: ĐO THỜI GIAN TÍNH TOÁN BẰNG MPI_Wtime ---
    const double compute_start = MPI_Wtime();

    const int interior_size = n - 2;
    const double mu = compute_mu(config);

    std::vector<double> lower(interior_size, -mu);
    std::vector<double> diagonal(interior_size, 1.0 + 2.0 * mu);
    std::vector<double> upper(interior_size, -mu);
    std::vector<double> rhs(interior_size, 0.0);
    std::vector<double> solution(interior_size, 0.0);

    // Tạo bộ đệm local riêng để chứa kết quả tính toán của Rank này
    std::vector<double> local_buffer(decomposition.local_count * n, 0.0);

    for (int i = 0; i < decomposition.local_count; ++i)
    {
        int row = decomposition.local_start + i;
        
        // Điều kiện biên Dirichlet = 0 ở hàng đầu và hàng cuối
        if (row == 0 || row == n - 1) {
            continue;
        }

        for (int col = 1; col < n - 1; ++col)
        {
            const double dx_previous = u_previous(row, col + 1) - 2.0 * u_previous(row, col) + u_previous(row, col - 1);
            const double dy_previous = u_previous(row + 1, col) - 2.0 * u_previous(row, col) + u_previous(row - 1, col);
            
            rhs[col - 1] = 2.0 * u_current(row, col) - u_previous(row, col) + mu * (dx_previous + dy_previous);
        }

        if (!solve_tridiagonal(config.solver, lower, diagonal, upper, rhs, solution))
        {
            return false;
        }

        // Ghi kết quả vào bộ đệm gửi local_buffer
        for (int col = 1; col < n - 1; ++col)
        {
            local_buffer[i * n + col] = solution[col - 1];
        }
    }

    // Cộng dồn thời gian tính toán local bằng MPI_Wtime
    timing.compute_x += MPI_Wtime() - compute_start;

    // --- BƯỚC 2: CHUẨN BỊ MẢNG ĐẾM VÀ TRUYỀN THÔNG ---
    std::vector<int> recvcounts(world_size);
    std::vector<int> displs(world_size);

    for (int r = 0; r < world_size; ++r)
    {
        recvcounts[r] = decomposition.counts[r] * n;
        displs[r] = decomposition.displacements[r] * n;
    }

    // Đo thời gian truyền thông mạng bằng MPI_Wtime
    const double comm_start = MPI_Wtime();

    MPI_Allgatherv(
        local_buffer.data(),
        decomposition.local_count * n,
        MPI_DOUBLE,
        u_half.data(),
        recvcounts.data(),
        displs.data(),
        MPI_DOUBLE,
        communicator
    );

    timing.communication_x += MPI_Wtime() - comm_start;

    return true;
}