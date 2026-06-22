# Hướng dẫn thiết lập khung code dự án OpenMPI  
## Song song hóa phương trình sóng âm 2D bằng ADI + OpenMPI

Tài liệu này hướng dẫn nhóm 4 người thiết lập một repository chung để cùng phát triển chương trình giải phương trình sóng âm 2D theo hướng song song hóa dữ liệu bằng OpenMPI.

Mục tiêu của khung code:

- Có một chương trình tuần tự để làm kết quả chuẩn.
- Có một chương trình MPI chạy được trên 3 máy cùng mạng.
- Tách rõ X-sweep và Y-sweep để nhiều người có thể code song song.
- Đo riêng thời gian tính toán và truyền thông.
- Kiểm tra kết quả MPI với chương trình tuần tự.
- Sinh dữ liệu cho các biểu đồ runtime, speedup, efficiency, granularity và load balance.
- Mỗi thành viên đều có phần code độc lập nhưng dùng chung interface.

---

# 1. Phạm vi triển khai

Nhóm triển khai bộ giải phương trình sóng âm 2D với luồng:

```text
Khởi tạo bài toán
        |
        v
ADI X-sweep
Giải nhiều hệ tam đường chéo theo hàng
        |
        v
MPI_Allgatherv
Ghép nghiệm trung gian U_half
        |
        v
ADI Y-sweep
Giải nhiều hệ tam đường chéo theo cột
        |
        v
MPI_Allgatherv
Ghép nghiệm U_next
        |
        v
Cập nhật bước thời gian
```

Hai bộ giải hệ tam đường chéo cần hỗ trợ:

```text
1. Thomas algorithm
2. Cyclic reduction
```

Hai chiến lược giao tiếp MPI cần hỗ trợ:

```text
1. blocking
2. nonblocking
```

Lưu ý:

- Không cần triển khai CUDA.
- Không cần triển khai GPU.
- Không cần giữ kiến trúc master chỉ quản lý.
- Rank 0 vẫn tham gia tính toán như các rank khác.
- Mỗi rank có thể giữ toàn bộ ma trận để đơn giản hóa việc triển khai.

---

# 2. Công nghệ và yêu cầu môi trường

## 2.1. Ngôn ngữ và thư viện

- C++17
- OpenMPI
- CMake
- Python 3
- NumPy
- pandas
- matplotlib
- Git

## 2.2. Hệ điều hành khuyến nghị

Khuyến nghị dùng Ubuntu hoặc WSL2 Ubuntu trên cả ba máy.

Ví dụ:

```text
Ubuntu 22.04
Ubuntu 24.04
```

Ba máy nên dùng cùng phiên bản OpenMPI hoặc ít nhất cùng major version.

Kiểm tra:

```bash
mpirun --version
mpicxx --version
cmake --version
g++ --version
```

---

# 3. Cài đặt môi trường trên từng máy

Chạy trên cả ba máy:

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    git \
    openmpi-bin \
    libopenmpi-dev \
    openssh-server \
    python3 \
    python3-pip \
    python3-venv
```

Cài Python packages:

```bash
python3 -m venv .venv
source .venv/bin/activate

pip install numpy pandas matplotlib
```

Kiểm tra MPI:

```bash
which mpirun
which mpicxx
mpirun --version
```

---

# 4. Cấu hình mạng giữa ba máy

Giả sử:

```text
Máy 1: master
IP: 192.168.1.10

Máy 2: slave1
IP: 192.168.1.11

Máy 3: slave2
IP: 192.168.1.12
```

Thay các IP trên bằng IP thật của nhóm.

## 4.1. Kiểm tra kết nối

Từ master:

```bash
ping 192.168.1.11
ping 192.168.1.12
```

Từ các máy slave:

```bash
ping 192.168.1.10
```

## 4.2. Gán hostname

Trên từng máy:

```bash
sudo hostnamectl set-hostname master
```

hoặc:

```bash
sudo hostnamectl set-hostname slave1
sudo hostnamectl set-hostname slave2
```

Thêm vào `/etc/hosts` trên cả ba máy:

```text
192.168.1.10 master
192.168.1.11 slave1
192.168.1.12 slave2
```

Kiểm tra:

```bash
ping master
ping slave1
ping slave2
```

---

# 5. Cấu hình SSH không cần mật khẩu

MPI cần SSH từ master tới các slave.

Trên master:

```bash
ssh-keygen -t ed25519
```

Nhấn Enter để dùng đường dẫn mặc định.

Copy key:

```bash
ssh-copy-id your_username@slave1
ssh-copy-id your_username@slave2
```

Kiểm tra:

```bash
ssh slave1 hostname
ssh slave2 hostname
```

Kết quả mong đợi:

```text
slave1
slave2
```

Không được yêu cầu nhập password.

Nếu username trên ba máy giống nhau thì hostfile sẽ đơn giản hơn.

---

# 6. Tạo repository chung

Trên master:

```bash
mkdir wave-mpi
cd wave-mpi
git init
```

Cấu trúc repository bắt buộc:

```text
wave-mpi/
├── CMakeLists.txt
├── README.md
├── hosts
├── config/
│   └── default.conf
├── include/
│   ├── common_types.hpp
│   ├── grid.hpp
│   ├── tridiagonal_solver.hpp
│   ├── wave_problem.hpp
│   ├── adi_serial.hpp
│   ├── decomposition.hpp
│   ├── mpi_x_sweep.hpp
│   ├── column_buffer.hpp
│   ├── mpi_communication.hpp
│   ├── mpi_y_sweep.hpp
│   ├── metrics.hpp
│   └── result_writer.hpp
├── src/
│   ├── core/
│   │   ├── grid.cpp
│   │   └── wave_problem.cpp
│   ├── solver/
│   │   ├── thomas.cpp
│   │   └── cyclic_reduction.cpp
│   ├── serial/
│   │   └── adi_serial.cpp
│   ├── mpi/
│   │   ├── decomposition.cpp
│   │   ├── mpi_x_sweep.cpp
│   │   ├── column_buffer.cpp
│   │   ├── communication_blocking.cpp
│   │   ├── communication_nonblocking.cpp
│   │   └── mpi_y_sweep.cpp
│   ├── benchmark/
│   │   ├── metrics.cpp
│   │   └── result_writer.cpp
│   ├── main_serial.cpp
│   └── main_mpi.cpp
├── tests/
│   ├── test_thomas.cpp
│   ├── test_cyclic_reduction.cpp
│   ├── test_decomposition.cpp
│   ├── test_column_buffer.cpp
│   ├── test_mpi_x_sweep.cpp
│   ├── test_mpi_y_sweep.cpp
│   └── test_metrics.cpp
├── scripts/
│   ├── sync_cluster.sh
│   ├── run_experiments.py
│   ├── plot_runtime.py
│   ├── plot_speedup.py
│   ├── plot_load_balance.py
│   └── calibrate_size.py
├── results/
│   ├── raw/
│   ├── summaries/
│   └── figures/
└── docs/
    ├── numerical_method.md
    ├── mpi_design.md
    └── experiment_plan.md
```

Tạo nhanh:

```bash
mkdir -p \
    config \
    include \
    src/core \
    src/solver \
    src/serial \
    src/mpi \
    src/benchmark \
    tests \
    scripts \
    results/raw \
    results/summaries \
    results/figures \
    docs
```

---

# 7. File `.gitignore`

Tạo `.gitignore`:

```gitignore
build/
.venv/
__pycache__/
*.pyc
*.o
*.out
*.log
*.bin

results/raw/*
results/figures/*
results/summaries/*

!results/raw/.gitkeep
!results/figures/.gitkeep
!results/summaries/.gitkeep
```

Tạo file giữ thư mục:

```bash
touch results/raw/.gitkeep
touch results/summaries/.gitkeep
touch results/figures/.gitkeep
```

---

# 8. Chuẩn kiểu dữ liệu dùng chung

Tạo file:

```text
include/common_types.hpp
```

Nội dung:

```cpp
#pragma once

#include <string>
#include <vector>

struct SimulationConfig {
    int grid_size = 255;
    int time_steps = 50;

    double domain_length = 1.0;
    double total_time = 1.0;
    double wave_speed = 1.0;

    std::string solver = "thomas";
    std::string communication = "blocking";

    std::string output_path = "";
    std::string reference_path = "";
};

struct TimingInfo {
    double initialization = 0.0;

    double compute_x = 0.0;
    double communication_x = 0.0;

    double compute_y = 0.0;
    double communication_y = 0.0;

    double waiting = 0.0;
    double total = 0.0;
};

struct ErrorMetrics {
    double max_absolute_error = 0.0;
    double l2_error = 0.0;
};

struct Decomposition {
    int global_size = 0;
    int local_start = 0;
    int local_count = 0;

    std::vector<int> counts;
    std::vector<int> displacements;
};
```

Quy tắc:

- Không tự ý sửa tên field.
- Không thêm field mà chưa thống nhất với nhóm.
- Mọi thời gian đo bằng giây.
- Mọi phép tính số thực dùng `double`.

---

# 9. Class Grid dùng chung

Tạo:

```text
include/grid.hpp
```

```cpp
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
```

Tạo:

```text
src/core/grid.cpp
```

```cpp
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
```

Quy ước bắt buộc:

```cpp
index = row * number_of_columns + column;
```

Không được dùng column-major.

---

# 10. Interface của bộ giải hệ tam đường chéo

Tạo:

```text
include/tridiagonal_solver.hpp
```

```cpp
#pragma once

#include <string>
#include <vector>

bool solve_thomas(
    const std::vector<double>& lower,
    const std::vector<double>& diagonal,
    const std::vector<double>& upper,
    const std::vector<double>& rhs,
    std::vector<double>& solution
);

bool solve_cyclic_reduction(
    const std::vector<double>& lower,
    const std::vector<double>& diagonal,
    const std::vector<double>& upper,
    const std::vector<double>& rhs,
    std::vector<double>& solution
);

bool solve_tridiagonal(
    const std::string& solver_name,
    const std::vector<double>& lower,
    const std::vector<double>& diagonal,
    const std::vector<double>& upper,
    const std::vector<double>& rhs,
    std::vector<double>& solution
);
```

Người phụ trách numerical core phải đảm bảo:

```text
solver_name = "thomas"
solver_name = "cr"
```

đều được hỗ trợ.

---

# 11. Interface khởi tạo bài toán

Tạo:

```text
include/wave_problem.hpp
```

```cpp
#pragma once

#include "common_types.hpp"
#include "grid.hpp"

void initialize_wave_problem(
    Grid& u_previous,
    Grid& u_current,
    const SimulationConfig& config
);

void apply_boundary_conditions(Grid& grid);

double exact_solution(
    double t,
    double x,
    double y,
    const SimulationConfig& config
);
```

Điều kiện đầu nên dùng:

```math
u(0,x,y) = sin(pi x) sin(pi y)
```

và:

```math
du/dt = 0
```

Điều kiện biên:

```math
u(t,0,y) = 0
u(t,L,y) = 0
u(t,x,0) = 0
u(t,x,L) = 0
```

---

# 12. Interface serial solver

Tạo:

```text
include/adi_serial.hpp
```

```cpp
#pragma once

#include "common_types.hpp"
#include "grid.hpp"

bool serial_x_sweep(
    const Grid& u_previous,
    const Grid& u_current,
    Grid& u_half,
    const SimulationConfig& config,
    TimingInfo& timing
);

bool serial_y_sweep(
    const Grid& u_half,
    const Grid& u_current,
    Grid& u_next,
    const SimulationConfig& config,
    TimingInfo& timing
);

bool solve_wave_serial(
    const SimulationConfig& config,
    Grid& final_result,
    TimingInfo& timing
);
```

Chương trình tuần tự là kết quả chuẩn để kiểm tra chương trình MPI.

---

# 13. Interface data decomposition

Tạo:

```text
include/decomposition.hpp
```

```cpp
#pragma once

#include "common_types.hpp"

Decomposition create_block_decomposition(
    int global_size,
    int rank,
    int world_size
);
```

Quy tắc chia block:

```cpp
base = global_size / world_size;
remainder = global_size % world_size;
```

Với rank nhỏ hơn `remainder`:

```cpp
local_count = base + 1;
```

Các rank còn lại:

```cpp
local_count = base;
```

Yêu cầu:

- Không mất phần tử.
- Không trùng phần tử.
- Chênh lệch số phần tử giữa hai rank tối đa bằng 1.

---

# 14. Interface MPI X-sweep

Tạo:

```text
include/mpi_x_sweep.hpp
```

```cpp
#pragma once

#include <mpi.h>

#include "common_types.hpp"
#include "grid.hpp"

bool mpi_x_sweep(
    const Grid& u_previous,
    const Grid& u_current,
    Grid& u_half,
    const SimulationConfig& config,
    const Decomposition& decomposition,
    MPI_Comm communicator,
    TimingInfo& timing
);
```

Quy ước:

- Mỗi rank xử lý một nhóm hàng liên tục.
- Mỗi hàng tương ứng với một hệ tam đường chéo.
- Sau khi tính xong, dùng `MPI_Allgatherv`.
- Sau khi hàm kết thúc, mọi rank có cùng `u_half`.

---

# 15. Interface đóng gói cột

Tạo:

```text
include/column_buffer.hpp
```

```cpp
#pragma once

#include <vector>

#include "grid.hpp"

void pack_columns(
    const Grid& input,
    int start_column,
    int column_count,
    std::vector<double>& buffer
);

void unpack_columns(
    const std::vector<double>& buffer,
    int start_column,
    int column_count,
    Grid& output
);
```

Quy ước buffer:

```cpp
buffer[local_column * number_of_rows + row]
```

Ví dụ:

```text
Cột local 0:
buffer[0 * rows + 0]
buffer[0 * rows + 1]
buffer[0 * rows + 2]

Cột local 1:
buffer[1 * rows + 0]
buffer[1 * rows + 1]
buffer[1 * rows + 2]
```

---

# 16. Interface truyền thông MPI

Tạo:

```text
include/mpi_communication.hpp
```

```cpp
#pragma once

#include <mpi.h>

#include <string>
#include <vector>

bool gather_distributed_data(
    const std::vector<double>& local_buffer,
    std::vector<double>& global_buffer,
    const std::vector<int>& counts,
    const std::vector<int>& displacements,
    const std::string& communication_mode,
    MPI_Comm communicator,
    double& communication_time
);
```

Hai implementation:

```text
src/mpi/communication_blocking.cpp
src/mpi/communication_nonblocking.cpp
```

Blocking:

```cpp
MPI_Allgatherv(...)
```

Nonblocking:

```cpp
MPI_Iallgatherv(...)
MPI_Wait(...)
```

Lưu ý:

- Tất cả rank phải gọi collective.
- Không đặt collective bên trong `if (rank == 0)`.
- Phải đo riêng communication time.
- Nonblocking phải luôn gọi `MPI_Wait`.

---

# 17. Interface MPI Y-sweep

Tạo:

```text
include/mpi_y_sweep.hpp
```

```cpp
#pragma once

#include <mpi.h>

#include "common_types.hpp"
#include "grid.hpp"

bool mpi_y_sweep(
    const Grid& u_half,
    const Grid& u_current,
    Grid& u_next,
    const SimulationConfig& config,
    const Decomposition& decomposition,
    MPI_Comm communicator,
    TimingInfo& timing
);
```

Quy ước:

- Mỗi rank xử lý một nhóm cột.
- Dùng `pack_columns`.
- Giải từng hệ tam đường chéo.
- Dùng communication interface để ghép dữ liệu.
- Dùng `unpack_columns`.
- Sau khi hàm kết thúc, mọi rank có cùng `u_next`.

---

# 18. Interface metrics

Tạo:

```text
include/metrics.hpp
```

```cpp
#pragma once

#include <vector>

#include "common_types.hpp"
#include "grid.hpp"

ErrorMetrics compare_grids(
    const Grid& reference,
    const Grid& candidate
);

double calculate_speedup(
    double serial_time,
    double parallel_time
);

double calculate_efficiency(
    double speedup,
    int process_count
);

double calculate_communication_ratio(
    const TimingInfo& timing
);

double calculate_load_imbalance(
    const std::vector<double>& rank_times
);
```

Công thức:

```math
speedup = T_serial / T_parallel
```

```math
efficiency = speedup / number_of_processes
```

```math
communication_ratio =
(communication_x + communication_y) / total
```

```math
load_imbalance =
(max_time - average_time) / average_time
```

---

# 19. Interface ghi kết quả

Tạo:

```text
include/result_writer.hpp
```

```cpp
#pragma once

#include <string>
#include <vector>

#include "common_types.hpp"
#include "grid.hpp"

bool write_grid_csv(
    const Grid& grid,
    const std::string& output_path
);

bool append_benchmark_summary(
    const SimulationConfig& config,
    const TimingInfo& timing,
    const ErrorMetrics& errors,
    int process_count,
    int machine_count,
    double speedup,
    double efficiency,
    const std::string& status,
    const std::string& output_path
);

bool write_rank_timing_csv(
    const std::vector<TimingInfo>& rank_timings,
    const std::vector<std::string>& hostnames,
    const std::vector<Decomposition>& decompositions,
    const std::string& output_path
);
```

---

# 20. Khung `main_serial.cpp`

Tạo:

```text
src/main_serial.cpp
```

Khung tham khảo:

```cpp
#include <iostream>

#include "adi_serial.hpp"
#include "common_types.hpp"
#include "grid.hpp"
#include "result_writer.hpp"

int main(int argc, char** argv) {
    SimulationConfig config;

    // TODO:
    // parse command line arguments

    Grid final_result;
    TimingInfo timing;

    const bool success = solve_wave_serial(
        config,
        final_result,
        timing
    );

    if (!success) {
        std::cerr << "status=failed\n";
        return 1;
    }

    if (!config.output_path.empty()) {
        write_grid_csv(final_result, config.output_path);
    }

    std::cout << "mode=serial\n";
    std::cout << "solver=" << config.solver << "\n";
    std::cout << "grid_size=" << config.grid_size << "\n";
    std::cout << "time_steps=" << config.time_steps << "\n";
    std::cout << "status=success\n";
    std::cout << "runtime_seconds=" << timing.total << "\n";

    return 0;
}
```

---

# 21. Khung `main_mpi.cpp`

Tạo:

```text
src/main_mpi.cpp
```

Khung tham khảo:

```cpp
#include <mpi.h>

#include <iostream>
#include <string>
#include <vector>

#include "common_types.hpp"
#include "decomposition.hpp"
#include "grid.hpp"
#include "metrics.hpp"
#include "mpi_x_sweep.hpp"
#include "mpi_y_sweep.hpp"
#include "result_writer.hpp"
#include "wave_problem.hpp"

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = 0;
    int world_size = 1;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    SimulationConfig config;

    // TODO:
    // Rank 0 parse command line.
    // Broadcast config tới tất cả rank.

    const Decomposition decomposition =
        create_block_decomposition(
            config.grid_size,
            rank,
            world_size
        );

    Grid u_previous;
    Grid u_current;
    Grid u_half;
    Grid u_next;

    initialize_wave_problem(
        u_previous,
        u_current,
        config
    );

    u_half.resize(
        config.grid_size,
        config.grid_size
    );

    u_next.resize(
        config.grid_size,
        config.grid_size
    );

    TimingInfo local_timing;

    MPI_Barrier(MPI_COMM_WORLD);
    const double total_start = MPI_Wtime();

    bool success = true;

    for (int step = 0; step < config.time_steps; ++step) {
        success = mpi_x_sweep(
            u_previous,
            u_current,
            u_half,
            config,
            decomposition,
            MPI_COMM_WORLD,
            local_timing
        );

        if (!success) {
            break;
        }

        success = mpi_y_sweep(
            u_half,
            u_current,
            u_next,
            config,
            decomposition,
            MPI_COMM_WORLD,
            local_timing
        );

        if (!success) {
            break;
        }

        u_previous = u_current;
        u_current = u_next;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    local_timing.total = MPI_Wtime() - total_start;

    double global_total = 0.0;

    MPI_Reduce(
        &local_timing.total,
        &global_total,
        1,
        MPI_DOUBLE,
        MPI_MAX,
        0,
        MPI_COMM_WORLD
    );

    if (rank == 0) {
        std::cout << "mode=mpi\n";
        std::cout << "grid_size=" << config.grid_size << "\n";
        std::cout << "time_steps=" << config.time_steps << "\n";
        std::cout << "processes=" << world_size << "\n";
        std::cout << "solver=" << config.solver << "\n";
        std::cout << "communication="
                  << config.communication << "\n";
        std::cout << "status="
                  << (success ? "success" : "failed")
                  << "\n";
        std::cout << "total_seconds="
                  << global_total << "\n";
    }

    MPI_Finalize();
    return success ? 0 : 1;
}
```

Lưu ý:

- Không được để rank nào thoát trước `MPI_Finalize`.
- Nếu có lỗi, nên dùng `MPI_Allreduce` để đồng bộ trạng thái lỗi.
- Runtime chính là `MPI_MAX` của các rank.
- Không dùng runtime riêng của rank 0 làm runtime toàn chương trình.

---

# 22. File CMakeLists.txt

Tạo file gốc:

```text
CMakeLists.txt
```

Nội dung khung:

```cmake
cmake_minimum_required(VERSION 3.16)

project(wave_mpi LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(MPI REQUIRED)

add_library(wave_core
    src/core/grid.cpp
    src/core/wave_problem.cpp
    src/solver/thomas.cpp
    src/solver/cyclic_reduction.cpp
    src/serial/adi_serial.cpp
    src/mpi/decomposition.cpp
    src/mpi/mpi_x_sweep.cpp
    src/mpi/column_buffer.cpp
    src/mpi/communication_blocking.cpp
    src/mpi/communication_nonblocking.cpp
    src/mpi/mpi_y_sweep.cpp
    src/benchmark/metrics.cpp
    src/benchmark/result_writer.cpp
)

target_include_directories(wave_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(wave_core
    PUBLIC
        MPI::MPI_CXX
)

target_compile_options(wave_core
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -O3
)

add_executable(wave_serial
    src/main_serial.cpp
)

target_link_libraries(wave_serial
    PRIVATE
        wave_core
)

add_executable(wave_mpi
    src/main_mpi.cpp
)

target_link_libraries(wave_mpi
    PRIVATE
        wave_core
        MPI::MPI_CXX
)

enable_testing()

add_executable(test_thomas tests/test_thomas.cpp)
target_link_libraries(test_thomas PRIVATE wave_core)
add_test(NAME test_thomas COMMAND test_thomas)

add_executable(
    test_cyclic_reduction
    tests/test_cyclic_reduction.cpp
)
target_link_libraries(
    test_cyclic_reduction
    PRIVATE wave_core
)
add_test(
    NAME test_cyclic_reduction
    COMMAND test_cyclic_reduction
)

add_executable(
    test_decomposition
    tests/test_decomposition.cpp
)
target_link_libraries(
    test_decomposition
    PRIVATE wave_core
)
add_test(
    NAME test_decomposition
    COMMAND test_decomposition
)

add_executable(
    test_column_buffer
    tests/test_column_buffer.cpp
)
target_link_libraries(
    test_column_buffer
    PRIVATE wave_core
)
add_test(
    NAME test_column_buffer
    COMMAND test_column_buffer
)

add_executable(
    test_metrics
    tests/test_metrics.cpp
)
target_link_libraries(
    test_metrics
    PRIVATE wave_core
)
add_test(
    NAME test_metrics
    COMMAND test_metrics
)

add_executable(
    test_mpi_x_sweep
    tests/test_mpi_x_sweep.cpp
)
target_link_libraries(
    test_mpi_x_sweep
    PRIVATE
        wave_core
        MPI::MPI_CXX
)

add_executable(
    test_mpi_y_sweep
    tests/test_mpi_y_sweep.cpp
)
target_link_libraries(
    test_mpi_y_sweep
    PRIVATE
        wave_core
        MPI::MPI_CXX
)
```

---

# 23. Build chương trình

Từ thư mục root:

```bash
mkdir -p build
cd build

cmake ..
cmake --build . -j
```

Kết quả:

```text
build/wave_serial
build/wave_mpi
build/test_thomas
build/test_cyclic_reduction
build/test_decomposition
build/test_column_buffer
build/test_metrics
build/test_mpi_x_sweep
build/test_mpi_y_sweep
```

Chạy test thường:

```bash
ctest --output-on-failure
```

Chạy test MPI:

```bash
mpirun -np 3 ./test_mpi_x_sweep
mpirun -np 3 ./test_mpi_y_sweep
```

---

# 24. File hostfile

Tạo file:

```text
hosts
```

Ví dụ:

```text
master slots=4
slave1 slots=4
slave2 slots=4
```

`slots` là số MPI process tối đa mong muốn trên từng máy.

Nếu cấu hình máy khác nhau:

```text
master slots=4
slave1 slots=8
slave2 slots=2
```

Kiểm tra:

```bash
mpirun \
    --hostfile hosts \
    -np 3 \
    hostname
```

Kết quả phải có hostname của ba máy.

---

# 25. Đồng bộ code sang các máy

Cách đơn giản nhất là dùng Git.

Trên master:

```bash
git add .
git commit -m "Initialize project framework"
```

Push lên GitHub hoặc GitLab private repository.

Trên từng slave:

```bash
git clone <repository-url>
cd wave-mpi
mkdir build
cd build
cmake ..
cmake --build . -j
```

Một lựa chọn khác là dùng `rsync`.

Tạo:

```text
scripts/sync_cluster.sh
```

```bash
#!/usr/bin/env bash

set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

rsync -az \
    --delete \
    --exclude build \
    --exclude .git \
    "$PROJECT_DIR/" \
    slave1:~/wave-mpi/

rsync -az \
    --delete \
    --exclude build \
    --exclude .git \
    "$PROJECT_DIR/" \
    slave2:~/wave-mpi/

echo "Cluster synchronization completed."
```

Cho phép chạy:

```bash
chmod +x scripts/sync_cluster.sh
```

Chạy:

```bash
./scripts/sync_cluster.sh
```

Yêu cầu:

- Project phải nằm cùng đường dẫn trên ba máy.
- Ví dụ: `~/wave-mpi`.
- File executable phải tồn tại trên cả ba máy.

---

# 26. Chạy chương trình trên một máy

Serial:

```bash
./build/wave_serial \
    --grid-size 255 \
    --time-steps 50 \
    --solver thomas \
    --output results/raw/serial_N255.csv
```

MPI trên một máy:

```bash
mpirun -np 4 \
    ./build/wave_mpi \
    --grid-size 255 \
    --time-steps 50 \
    --solver thomas \
    --communication blocking
```

---

# 27. Chạy MPI trên ba máy

Từ master:

```bash
mpirun \
    --hostfile hosts \
    -np 6 \
    ./build/wave_mpi \
    --grid-size 511 \
    --time-steps 50 \
    --solver thomas \
    --communication blocking \
    --reference results/raw/serial_N511.csv \
    --output results/raw/mpi_N511_P6.csv
```

Nếu MPI không tìm thấy executable:

```bash
mpirun \
    --hostfile hosts \
    -np 6 \
    --wd ~/wave-mpi \
    ./build/wave_mpi
```

Nếu OpenMPI chặn oversubscription:

```bash
mpirun \
    --oversubscribe \
    --hostfile hosts \
    -np 24 \
    ./build/wave_mpi
```

---

# 28. Chuẩn command line

Cả serial và MPI nên hỗ trợ:

```text
--grid-size
--time-steps
--domain-length
--total-time
--wave-speed
--solver
--communication
--reference
--output
```

Ví dụ:

```bash
./build/wave_serial \
    --grid-size 511 \
    --time-steps 50 \
    --domain-length 1.0 \
    --total-time 1.0 \
    --wave-speed 1.0 \
    --solver thomas \
    --output results/raw/serial_N511.csv
```

MPI:

```bash
mpirun \
    --hostfile hosts \
    -np 8 \
    ./build/wave_mpi \
    --grid-size 511 \
    --time-steps 50 \
    --solver cr \
    --communication nonblocking \
    --reference results/raw/serial_N511.csv \
    --output results/raw/mpi_N511_P8.csv
```

---

# 29. Chuẩn console output

Serial:

```text
mode=serial
solver=thomas
grid_size=511
time_steps=50
status=success
runtime_seconds=67.413285
output=results/raw/serial_N511.csv
```

MPI:

```text
mode=mpi
grid_size=511
time_steps=50
processes=6
solver=thomas
communication=blocking
status=success

total_seconds=21.472819
compute_x_seconds=8.173622
communication_x_seconds=1.922831
compute_y_seconds=8.494211
communication_y_seconds=2.315421
waiting_seconds=0.566734

max_absolute_error=4.32e-10
l2_error=7.91e-11
correctness=passed
```

Chỉ rank 0 in summary.

Debug log từng rank nên bật bằng option riêng, ví dụ:

```text
--verbose
```

---

# 30. Chuẩn file output

## 30.1. Ma trận kết quả

```csv
row,column,value
0,0,0.000000000000
0,1,0.000000000000
1,1,0.002348276421
```

## 30.2. Benchmark summary

File:

```text
results/summaries/benchmark_summary.csv
```

Header:

```csv
run_id,timestamp,grid_size,time_steps,processes,machines,solver,communication,total_time,compute_x,communication_x,compute_y,communication_y,waiting_time,max_error,l2_error,speedup,efficiency,status
```

## 30.3. Rank timing

File:

```text
results/summaries/rank_timing_N511_P6.csv
```

Header:

```csv
run_id,rank,hostname,local_start,local_count,compute_x,communication_x,compute_y,communication_y,total_time
```

---

# 31. Chuẩn unit test cho từng module

## 31.1. Thomas

Test ít nhất ba hệ:

```text
Hệ nhỏ 3x3
Hệ diagonal dominant
Hệ ngẫu nhiên có nghiệm biết trước
```

Tiêu chí:

```text
max_error < 1e-10
```

## 31.2. Cyclic reduction

So sánh với Thomas:

```text
max |x_CR - x_Thomas| < 1e-9
```

## 31.3. Decomposition

Test:

```text
N=10, P=3
N=11, P=4
N=4, P=8
```

Kiểm tra:

- Tổng local_count bằng N.
- Không overlap.
- Không thiếu index.
- Chênh lệch load tối đa 1.

## 31.4. Pack/unpack columns

Tạo ma trận:

```text
1 2 3
4 5 6
7 8 9
```

Pack cột 1 phải ra:

```text
2 5 8
```

Pack rồi unpack phải khôi phục đúng dữ liệu.

## 31.5. X-sweep MPI

Chạy:

```bash
mpirun -np 1 ./test_mpi_x_sweep
mpirun -np 2 ./test_mpi_x_sweep
mpirun -np 3 ./test_mpi_x_sweep
```

So sánh với X-sweep serial:

```text
max_error < 1e-8
```

## 31.6. Y-sweep MPI

Chạy:

```bash
mpirun -np 1 ./test_mpi_y_sweep
mpirun -np 2 ./test_mpi_y_sweep
mpirun -np 3 ./test_mpi_y_sweep
```

So sánh với Y-sweep serial:

```text
max_error < 1e-8
```

## 31.7. Blocking và nonblocking

Hai mode phải cho kết quả tương đương:

```text
max_error < 1e-10
```

---

# 32. Phân chia công việc cho 4 người

## Người 1: Numerical core

Branch:

```text
feature/numerical-core
```

Phụ trách:

```text
include/tridiagonal_solver.hpp
include/wave_problem.hpp
include/adi_serial.hpp

src/solver/thomas.cpp
src/solver/cyclic_reduction.cpp
src/core/wave_problem.cpp
src/serial/adi_serial.cpp

tests/test_thomas.cpp
tests/test_cyclic_reduction.cpp
```

Đầu ra:

- `wave_serial` chạy được.
- Có kết quả serial chuẩn.
- Thomas và cyclic reduction vượt unit test.

## Người 2: Data decomposition và X-sweep

Branch:

```text
feature/mpi-x-sweep
```

Phụ trách:

```text
include/decomposition.hpp
include/mpi_x_sweep.hpp

src/mpi/decomposition.cpp
src/mpi/mpi_x_sweep.cpp

tests/test_decomposition.cpp
tests/test_mpi_x_sweep.cpp
```

Đầu ra:

- Chia hàng đúng.
- X-sweep MPI khớp serial.
- Đo riêng compute_x và communication_x.

## Người 3: Y-sweep và communication

Branch:

```text
feature/mpi-y-communication
```

Phụ trách:

```text
include/column_buffer.hpp
include/mpi_communication.hpp
include/mpi_y_sweep.hpp

src/mpi/column_buffer.cpp
src/mpi/communication_blocking.cpp
src/mpi/communication_nonblocking.cpp
src/mpi/mpi_y_sweep.cpp

tests/test_column_buffer.cpp
tests/test_mpi_y_sweep.cpp
```

Đầu ra:

- Pack/unpack đúng.
- Y-sweep MPI khớp serial.
- Blocking và nonblocking cho kết quả tương đương.

## Người 4: Main, metrics và benchmark

Branch:

```text
feature/benchmark-integration
```

Phụ trách:

```text
src/main_serial.cpp
src/main_mpi.cpp

include/metrics.hpp
include/result_writer.hpp

src/benchmark/metrics.cpp
src/benchmark/result_writer.cpp

scripts/run_experiments.py
scripts/plot_runtime.py
scripts/plot_speedup.py
scripts/plot_load_balance.py
```

Đầu ra:

- Ghép chương trình hoàn chỉnh.
- Chạy được trên ba máy.
- Sinh CSV và biểu đồ.
- Kiểm tra correctness tự động.

---

# 33. Quy trình Git bắt buộc

Mỗi người tạo branch riêng:

```bash
git checkout -b feature/numerical-core
```

Commit nhỏ, rõ nghĩa:

```bash
git add .
git commit -m "Implement Thomas tridiagonal solver"
```

Trước khi mở pull request:

```bash
git checkout main
git pull

git checkout feature/numerical-core
git rebase main
```

Checklist pull request:

```text
[ ] Build thành công
[ ] Unit test thành công
[ ] Không sửa interface chung tùy ý
[ ] Có sample command
[ ] Có sample output
[ ] Không commit file build
[ ] Không commit dữ liệu benchmark lớn
[ ] Không có biến global
[ ] Không gọi MPI_Init ngoài main_mpi.cpp
[ ] Không gọi MPI_Finalize ngoài main_mpi.cpp
```

---

# 34. Quy tắc tích hợp code

Các phần sau không được thay đổi nếu chưa được nhóm đồng ý:

```text
Grid
SimulationConfig
TimingInfo
ErrorMetrics
Decomposition
Public function signatures
Row-major memory layout
CSV headers
Correctness thresholds
```

Nguyên tắc:

- Public header là hợp đồng chung.
- Implementation có thể thay đổi.
- Không gọi trực tiếp code private của module khác.
- Không tự tạo class Grid thứ hai.
- Không tự tạo format output khác.
- Không sửa tên solver hoặc communication mode.

---

# 35. Kế hoạch tích hợp theo ngày

## Ngày 1

- Tạo repository.
- Tạo folder.
- Tạo header chung.
- Tạo CMakeLists.
- Chạy MPI hello world trên ba máy.

## Ngày 2–4

- Người 1 làm numerical core.
- Người 2 làm decomposition và X-sweep.
- Người 3 làm column buffer và communication.
- Người 4 làm main skeleton, metrics và CSV writer.

## Ngày 5

- Merge Grid và common types.
- Merge Thomas solver.
- Merge decomposition.
- Chạy unit test.

## Ngày 6–7

- Tích hợp X-sweep.
- Tích hợp Y-sweep.
- Chạy `-np 1`.

## Ngày 8

- Chạy `-np 2`, `-np 3`, `-np 4`.
- Kiểm tra deadlock.
- Kiểm tra kết quả.

## Ngày 9

- Chạy trên ba máy.
- So sánh blocking và nonblocking.

## Ngày 10

- Hoàn thành correctness test.
- Hoàn thành analytic solution test.

## Ngày 11–12

- Chạy benchmark.
- Mỗi cấu hình chạy ít nhất 5 lần.
- Lưu median.

## Ngày 13

- Vẽ biểu đồ.
- Phân tích communication, granularity và load balance.

## Ngày 14

- Hoàn thành báo cáo.
- Chạy demo cuối.
- Đóng version nộp bài.

---

# 36. Thí nghiệm bắt buộc

## 36.1. Runtime theo kích thước

Ví dụ:

```text
N = 127
N = 255
N = 511
N = 1023
N = 2047
```

Giữ số process cố định.

Vẽ:

```text
total runtime
compute-only runtime
communication runtime
```

## 36.2. Speedup

Ví dụ:

```text
P = 1, 2, 4, 8, 16
```

Công thức:

```math
S(P) = T(1) / T(P)
```

## 36.3. Efficiency

```math
E(P) = S(P) / P
```

## 36.4. Granularity

```math
G = N^2 / P
```

Phân tích khi P tăng:

- Công việc trên mỗi process giảm.
- Communication chiếm tỉ trọng lớn hơn.
- Speedup có thể bão hòa hoặc giảm.

## 36.5. Load balance

Mỗi rank xuất:

```text
rank
hostname
local_count
compute_x
compute_y
communication
total
```

## 36.6. Blocking và nonblocking

So sánh:

```text
MPI_Allgatherv
MPI_Iallgatherv + MPI_Wait
```

---

# 37. Script benchmark

Tạo:

```text
scripts/run_experiments.py
```

Logic đề xuất:

```python
grid_sizes = [127, 255, 511, 1023]
process_counts = [1, 2, 4, 8]
communication_modes = ["blocking", "nonblocking"]
repeat_count = 5
```

Với mỗi cấu hình:

```text
1. Chạy chương trình.
2. Ghi stdout.
3. Parse key=value.
4. Lưu CSV.
5. Lặp 5 lần.
6. Tính median.
```

Không nhập số liệu thủ công.

---

# 38. Quy tắc đo thời gian

Dùng:

```cpp
MPI_Wtime()
```

Trước benchmark:

```cpp
MPI_Barrier(MPI_COMM_WORLD);
```

Đo compute:

```cpp
const double start = MPI_Wtime();

// local computation

timing.compute_x += MPI_Wtime() - start;
```

Đo communication:

```cpp
const double start = MPI_Wtime();

MPI_Allgatherv(...);

timing.communication_x += MPI_Wtime() - start;
```

Runtime toàn chương trình:

```cpp
MPI_Reduce(
    &local_total,
    &global_total,
    1,
    MPI_DOUBLE,
    MPI_MAX,
    0,
    MPI_COMM_WORLD
);
```

Không dùng average runtime làm runtime chính.

---

# 39. Quy tắc kiểm tra correctness

So sánh MPI với serial:

```math
max_error =
max |u_MPI - u_serial|
```

```math
L2 =
sqrt(
    sum((u_MPI - u_serial)^2) / N^2
)
```

Ngưỡng đề xuất:

```text
max_error < 1e-8
```

Không yêu cầu giống tuyệt đối từng bit.

Kiểm tra thêm:

```text
Không NaN
Không Inf
Boundary bằng 0
Grid dimensions đúng
Blocking và nonblocking tương đương
```

---

# 40. Các lỗi phổ biến cần tránh

## Collective chỉ được gọi bởi rank 0

Sai:

```cpp
if (rank == 0) {
    MPI_Bcast(...);
}
```

Đúng:

```cpp
MPI_Bcast(...);
```

Mọi rank trong communicator phải gọi collective.

## Dùng số phần tử sai trong Allgatherv

`counts` và `displacements` phải tính theo số phần tử `double`, không phải số hàng hoặc số cột nếu buffer chứa nhiều phần tử.

Ví dụ:

```cpp
element_count = local_row_count * grid_size;
```

## Tất cả rank cùng ghi một file

Chỉ rank 0 ghi summary.

Nếu cần ghi file riêng:

```text
rank_0.csv
rank_1.csv
rank_2.csv
```

## Đường dẫn executable không giống nhau

Executable phải nằm cùng vị trí trên tất cả máy.

## Phiên bản OpenMPI khác nhau

Dùng cùng package và cùng phiên bản nếu có thể.

## Wi-Fi không ổn định

- Tắt tải file.
- Đặt ba máy gần router.
- Chạy mỗi cấu hình nhiều lần.
- Dùng median.
- Không đánh giá chỉ từ một lần chạy.

## Oversubscription

Khi số process vượt số core:

```bash
mpirun --oversubscribe ...
```

Speedup giảm là kết quả hợp lý, không phải lỗi.

---

# 41. Chuẩn bàn giao của mỗi thành viên

Mỗi người phải gửi:

```text
Tên module:
Người thực hiện:

Files added:
- ...

Public functions:
- ...

Build command:
- ...

Test command:
- ...

Test result:
- ...

Sample output:
- ...

Known limitations:
- ...
```

Ví dụ:

```text
Tên module: MPI X-sweep
Người thực hiện: Thành viên 2

Files added:
- include/mpi_x_sweep.hpp
- src/mpi/mpi_x_sweep.cpp
- tests/test_mpi_x_sweep.cpp

Public functions:
- create_block_decomposition()
- mpi_x_sweep()

Build command:
- cmake --build build -j

Test command:
- mpirun -np 3 ./build/test_mpi_x_sweep

Test result:
- decomposition_equal_work: passed
- x_sweep_matches_serial: passed
- max_error: 3.12e-11

Known limitations:
- Mỗi rank hiện giữ toàn bộ ma trận.
```

---

# 42. Definition of Done của toàn dự án

Dự án chỉ được coi là hoàn thành khi:

```text
[ ] Build thành công trên cả ba máy
[ ] Serial solver chạy được
[ ] MPI solver chạy được với -np 1
[ ] MPI solver chạy được với -np 2
[ ] MPI solver chạy được với -np 3
[ ] MPI solver chạy được trên ba máy
[ ] Thomas unit test passed
[ ] Cyclic reduction unit test passed
[ ] Decomposition unit test passed
[ ] Pack/unpack unit test passed
[ ] X-sweep MPI khớp serial
[ ] Y-sweep MPI khớp serial
[ ] Blocking và nonblocking tương đương
[ ] Không deadlock
[ ] Không NaN hoặc Inf
[ ] Boundary đúng
[ ] Có benchmark CSV
[ ] Có per-rank timing
[ ] Có biểu đồ runtime
[ ] Có biểu đồ speedup
[ ] Có biểu đồ efficiency
[ ] Có biểu đồ communication ratio
[ ] Có biểu đồ load balance
[ ] Có README chạy chương trình
[ ] Có báo cáo phân tích kết quả
```

---

# 43. Lệnh kiểm tra cuối cùng

Build:

```bash
cmake -S . -B build
cmake --build build -j
```

Unit test:

```bash
cd build
ctest --output-on-failure
```

MPI test:

```bash
mpirun -np 3 ./test_mpi_x_sweep
mpirun -np 3 ./test_mpi_y_sweep
```

Cluster test:

```bash
mpirun \
    --hostfile ../hosts \
    -np 3 \
    hostname
```

Chạy solver:

```bash
mpirun \
    --hostfile hosts \
    -np 6 \
    ./build/wave_mpi \
    --grid-size 511 \
    --time-steps 50 \
    --solver thomas \
    --communication blocking
```

Benchmark:

```bash
source .venv/bin/activate
python scripts/run_experiments.py
```

Vẽ biểu đồ:

```bash
python scripts/plot_runtime.py
python scripts/plot_speedup.py
python scripts/plot_load_balance.py
```

---

# 44. Kết luận

Khung code này được thiết kế để:

- Bốn người làm việc song song.
- Các module có interface rõ ràng.
- Giảm xung đột khi merge.
- Chạy được trên một máy và ba máy.
- Có đủ dữ liệu để đánh giá runtime, communication, speedup, granularity và load balance.
- Có chương trình tuần tự làm reference.
- Có unit test để phát hiện lỗi trước khi chạy benchmark.

Nhóm nên hoàn thành phần interface, CMake và repository structure trước khi bắt đầu viết thuật toán chi tiết. Đây là bước quan trọng nhất để bảo đảm bốn thành viên có thể code độc lập nhưng vẫn tích hợp được vào một chương trình chung.
