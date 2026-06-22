# wave-mpi

Song song hóa phương trình sóng âm 2D bằng **ADI + OpenMPI** (nhóm 4 người).

## Luồng xử lý

```text
Khởi tạo bài toán → ADI X-sweep → MPI_Allgatherv → ADI Y-sweep → MPI_Allgatherv → Cập nhật bước thời gian
```

## Yêu cầu môi trường

- C++17, CMake ≥ 3.16, OpenMPI
- Python 3 + NumPy + pandas + matplotlib (benchmark/plot)
- Khuyến nghị: Ubuntu 22.04/24.04 hoặc WSL2 trên cả 3 máy cluster

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Chạy thử

Serial (sau khi Người 1 hoàn thành solver):

```bash
./build/wave_serial --grid-size 255 --time-steps 50 --solver thomas --output results/raw/serial_N255.csv
```

MPI trên một máy:

```bash
mpirun -np 4 ./build/wave_mpi --grid-size 255 --time-steps 50 --solver thomas --communication blocking
```

MPI trên 3 máy (từ master):

```bash
mpirun --hostfile hosts -np 6 ./build/wave_mpi --grid-size 511 --time-steps 50 --solver thomas --communication blocking
```

## Unit test

```bash
cd build
ctest --output-on-failure
mpirun -np 3 ./test_mpi_x_sweep
mpirun -np 3 ./test_mpi_y_sweep
```

## Phân công nhóm

Chi tiết tại [docs/TEAM_ASSIGNMENTS.md](docs/TEAM_ASSIGNMENTS.md).

| Thành viên | Branch | Phụ trách chính |
|------------|--------|-----------------|
| Người 1 | `feature/numerical-core` | Thomas, CR, serial ADI, wave problem |
| Người 2 | `feature/mpi-x-sweep` | Decomposition, MPI X-sweep |
| Người 3 | `feature/mpi-y-communication` | Column buffer, MPI comm, Y-sweep |
| Người 4 | `feature/benchmark-integration` | main, metrics, CSV, benchmark scripts |

## Trạng thái khung code

Khung code đã có đủ header/interface, CMake, test skeleton và một số module nền (Grid, decomposition, column buffer, metrics). Các phần solver ADI và MPI sweep còn stub — mỗi thành viên triển khai theo branch của mình.

Tài liệu đầy đủ: [OpenMPI_2D_Wave_Project_Setup_Guide.md](../OpenMPI_2D_Wave_Project_Setup_Guide.md)
