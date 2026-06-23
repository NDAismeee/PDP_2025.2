#include <iostream>
#include <mpi.h>
#include <cassert>
#include <cmath>
#include "mpi_x_sweep.hpp"
#include "wave_problem.hpp"

int main(int argc, char** argv) {
    // Khởi tạo môi trường MPI
    MPI_Init(&argc, &argv);

    int rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // 1. Tạo cấu hình mô phỏng giả lập để test
    SimulationConfig config;
    config.grid_size = 100;         // Kích thước lưới 100x100
    config.domain_length = 1.0;
    config.total_time = 0.1;
    config.time_steps = 10;
    config.wave_speed = 1.0;
    config.solver = "thomas";       // Sử dụng bộ giải Thomas hoặc "cr" tùy cấu hình

    // 2. Khởi tạo các Grid dữ liệu (ma trận)
    Grid u_previous(config.grid_size, config.grid_size);
    Grid u_current(config.grid_size, config.grid_size);
    Grid u_half(config.grid_size, config.grid_size);

    // Điền dữ liệu giả lập ban đầu cho lưới (ví dụ: hàm sin hoặc giá trị khác 0)
    u_previous.fill(0.1);
    u_current.fill(0.2);

    // 3. Khởi tạo cấu trúc phân rã ma trận (Decomposition) theo hàng
    Decomposition decomposition;
    // Hàm phân chia dữ liệu này thường do cấu trúc của nhóm bạn viết sẵn
    // Dưới đây là cách tính toán phân chia cơ bản:
    decomposition.local_count = config.grid_size / world_size;
    decomposition.local_start = rank * decomposition.local_count;
    
    // Xử lý phần dư nếu lưới không chia hết cho số rank
    if (rank == world_size - 1) {
        decomposition.local_count += config.grid_size % world_size;
    }

    // Nạp mảng counts và displacements cho Allgatherv
    decomposition.counts.resize(world_size);
    decomposition.displacements.resize(world_size);
    for (int r = 0; r < world_size; ++r) {
        decomposition.counts[r] = config.grid_size / world_size;
        decomposition.displacements[r] = r * (config.grid_size / world_size);
        if (r == world_size - 1) {
            decomposition.counts[r] += config.grid_size % world_size;
        }
    }

    // Khởi tạo struct lưu thời gian chạy
    TimingInfo timing;
    timing.compute_x = 0.0;
    timing.communication_x = 0.0;

    // 4. Gọi hàm mpi_x_sweep mà bạn vừa code để thực hiện kiểm tra
    bool success = mpi_x_sweep(u_previous, u_current, u_half, config, decomposition, MPI_COMM_WORLD, timing);

    // 5. In kết quả kiểm tra
    if (rank == 0) {
        if (success) {
            std::cout << "test_mpi_x_sweep: PASSED thành công!\n";
            std::cout << "-> Thời gian tính toán X-sweep: " << timing.compute_x << " giây.\n";
            std::cout << "-> Thời gian truyền thông MPI: " << timing.communication_x << " giây.\n";
        } else {
            std::cout << "test_mpi_x_sweep: FAILED (Bộ giải hệ phương trình báo lỗi).\n";
        }
    }

    // Kết thúc môi trường MPI
    MPI_Finalize();
    return 0;
}