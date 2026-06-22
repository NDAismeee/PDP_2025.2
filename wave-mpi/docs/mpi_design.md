# Thiết kế MPI

## Phân rã dữ liệu

- **X-sweep:** chia theo hàng (block decomposition)
- **Y-sweep:** chia theo cột (block decomposition)
- Mỗi rank có thể giữ toàn bộ ma trận sau Allgatherv (đơn giản hóa triển khai)

## Giao tiếp

| Mode | API |
|------|-----|
| blocking | `MPI_Allgatherv` |
| nonblocking | `MPI_Iallgatherv` + `MPI_Wait` |

Interface chung: `gather_distributed_data()` trong `mpi_communication.hpp`.

## Đo thời gian

- Dùng `MPI_Wtime()`
- `MPI_Barrier` trước benchmark
- Runtime toàn chương trình = `MPI_MAX` trên các rank (không dùng thời gian rank 0)
- Tách riêng: `compute_x`, `communication_x`, `compute_y`, `communication_y`

## Cluster 3 máy

File `hosts`:

```text
master slots=4
slave1 slots=4
slave2 slots=4
```

Đồng bộ code: Git clone hoặc `scripts/sync_cluster.sh`.
