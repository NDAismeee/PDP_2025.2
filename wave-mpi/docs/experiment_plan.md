# Kế hoạch thí nghiệm

## 1. Runtime theo kích thước lưới

Giữ số process cố định, thay `N`:

```text
N = 127, 255, 511, 1023, 2047
```

Vẽ: total runtime, compute-only, communication.

## 2. Speedup và efficiency

```text
P = 1, 2, 4, 8, 16
S(P) = T(1) / T(P)
E(P) = S(P) / P
```

## 3. Granularity

```text
G = N² / P
```

Phân tích khi P tăng: communication ratio tăng, speedup bão hòa.

## 4. Load balance

Xuất per-rank timing CSV: rank, hostname, local_count, compute, communication, total.

## 5. Blocking vs nonblocking

So sánh `MPI_Allgatherv` và `MPI_Iallgatherv + MPI_Wait` trên cùng cấu hình.

## Quy tắc đo

- Mỗi cấu hình chạy ≥ 5 lần
- Lưu **median** (không nhập tay)
- Ngưỡng correctness: `max_error < 1e-8` so với serial

## Script

```bash
python scripts/run_experiments.py
python scripts/plot_runtime.py
python scripts/plot_speedup.py
python scripts/plot_load_balance.py
```
