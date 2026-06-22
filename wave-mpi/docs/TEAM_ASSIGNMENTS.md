# Phân chia công việc cho 4 thành viên

Repository: `wave-mpi`  
Hướng dẫn gốc: `OpenMPI_2D_Wave_Project_Setup_Guide.md`

---

## Người 1: Numerical core

**Branch:** `feature/numerical-core`

**Files phụ trách:**

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

**Đầu ra bắt buộc:**

- `wave_serial` chạy được end-to-end
- Kết quả serial làm chuẩn so sánh MPI
- Thomas và cyclic reduction (`solver=thomas`, `solver=cr`) pass unit test
- Điều kiện đầu: `u(0,x,y) = sin(πx) sin(πy)`, `du/dt = 0`
- Điều kiện biên Dirichlet = 0

**Lệnh kiểm tra:**

```bash
cmake --build build -j
./build/test_thomas
./build/test_cyclic_reduction
./build/wave_serial --grid-size 255 --time-steps 50 --solver thomas --output results/raw/serial_N255.csv
```

---

## Người 2: Data decomposition và X-sweep

**Branch:** `feature/mpi-x-sweep`

**Files phụ trách:**

```text
include/decomposition.hpp
include/mpi_x_sweep.hpp
src/mpi/decomposition.cpp
src/mpi/mpi_x_sweep.cpp
tests/test_decomposition.cpp
tests/test_mpi_x_sweep.cpp
```

**Đầu ra bắt buộc:**

- Chia block hàng đúng (load chênh lệch tối đa 1)
- `mpi_x_sweep` khớp `serial_x_sweep` (max_error < 1e-8)
- Đo riêng `compute_x` và `communication_x`
- Sau X-sweep mọi rank có cùng `u_half` (MPI_Allgatherv)

**Lệnh kiểm tra:**

```bash
./build/test_decomposition
mpirun -np 1 ./build/test_mpi_x_sweep
mpirun -np 2 ./build/test_mpi_x_sweep
mpirun -np 3 ./build/test_mpi_x_sweep
```

---

## Người 3: Y-sweep và communication

**Branch:** `feature/mpi-y-communication`

**Files phụ trách:**

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

**Đầu ra bắt buộc:**

- Pack/unpack cột đúng layout `buffer[local_column * rows + row]`
- `mpi_y_sweep` khớp `serial_y_sweep`
- Blocking (`MPI_Allgatherv`) và nonblocking (`MPI_Iallgatherv` + `MPI_Wait`) cho kết quả tương đương
- Mọi rank gọi collective, không đặt trong `if (rank == 0)`

**Lệnh kiểm tra:**

```bash
./build/test_column_buffer
mpirun -np 1 ./build/test_mpi_y_sweep
mpirun -np 3 ./build/test_mpi_y_sweep
```

---

## Người 4: Main, metrics và benchmark

**Branch:** `feature/benchmark-integration`

**Files phụ trách:**

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

**Đầu ra bắt buộc:**

- Parse CLI: `--grid-size`, `--time-steps`, `--solver`, `--communication`, `--reference`, `--output`, ...
- Rank 0 parse args, broadcast config cho mọi rank
- Console output đúng format key=value (xem hướng dẫn mục 29)
- Ghi CSV ma trận và `benchmark_summary.csv`
- Script benchmark tự động, lặp 5 lần, lấy median
- Biểu đồ runtime, speedup, efficiency, load balance

**Lệnh kiểm tra:**

```bash
./build/test_metrics
python scripts/run_experiments.py
python scripts/plot_runtime.py
```

---

## Quy trình Git

```bash
git checkout -b feature/<ten-branch>
# ... làm việc ...
git add .
git commit -m "Implement Thomas tridiagonal solver"

git checkout main && git pull
git checkout feature/<ten-branch>
git rebase main
# Mở PR, checklist trong hướng dẫn mục 33
```

## Không được sửa tùy ý (cần đồng thuận nhóm)

- `SimulationConfig`, `TimingInfo`, `ErrorMetrics`, `Decomposition`
- Chữ ký hàm public trong `include/`
- Row-major layout, CSV headers, ngưỡng correctness

## Kế hoạch tích hợp (14 ngày)

| Ngày | Việc chính |
|------|------------|
| 1 | Repo, folder, header, CMake, MPI hello trên 3 máy |
| 2–4 | Mỗi người code module riêng |
| 5 | Merge Grid, Thomas, decomposition; chạy unit test |
| 6–7 | Tích hợp X/Y-sweep, chạy `-np 1` |
| 8 | `-np 2,3,4`, kiểm tra deadlock |
| 9 | Chạy 3 máy, so sánh blocking/nonblocking |
| 10 | Correctness + analytic solution |
| 11–12 | Benchmark (≥5 lần/cấu hình, median) |
| 13 | Vẽ biểu đồ, phân tích |
| 14 | Báo cáo, demo, đóng version nộp |
