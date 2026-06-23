# Phương pháp số

## Bài toán

Phương trình sóng 2D trên miền vuông `[0, L] × [0, L]`:

```text
∂²u/∂t² = c² (∂²u/∂x² + ∂²u/∂y²)
```

## Điều kiện

- Ban đầu: `u(0,x,y) = sin(πx/L) sin(πy/L)`, `∂u/∂t(0,x,y) = 0`
- Biên Dirichlet: `u = 0` trên toàn bộ biên

## Quy ước lưới và thời gian

```text
h  = domain_length / (grid_size - 1)
dt = total_time / time_steps
μ  = 0.5 * (wave_speed * dt / h)²
```

Trong đó `wave_speed` là vận tốc `c`, và phương trình dùng `c²`.

## Khởi tạo hai mức thời gian

Phương trình sóng bậc hai cần hai mức ban đầu:

```text
u_previous = u⁰
u_current  = u¹
```

`u⁰` là điều kiện ban đầu. Vì `∂u/∂t(0) = 0`, mức `u¹` được tính bằng khai triển Taylor bậc hai:

```text
u¹ = u⁰ * [1 - (π c dt / L)²]
```

Sau khi khởi tạo, nếu `time_steps == 1` thì `u_current` đã là nghiệm tại `total_time`.

## ADI (Alternating Direction Implicit)

Mỗi bước thời gian (từ `step = 1` đến `step = time_steps - 1`) gồm:

1. **X-sweep:** giải hệ tam đường chéo theo từng hàng → `u_half`
2. **MPI_Allgatherv:** ghép `u_half` toàn cục (MPI)
3. **Y-sweep:** giải hệ tam đường chéo theo từng cột → `u_next`
4. **MPI_Allgatherv:** ghép `u_next` toàn cục (MPI)
5. Cập nhật `(u_previous, u_current)` cho bước tiếp theo

### X-sweep

Với mỗi hàng interior, tính RHS:

```text
R[i,j] = 2*uⁿ[i,j] - uⁿ⁻¹[i,j] + μ * (Dx uⁿ⁻¹[i,j] + Dy uⁿ⁻¹[i,j])
```

Giải hệ tam đường chéo theo cột (theo x):

```text
-μ u*[i,j-1] + (1+2μ) u*[i,j] - μ u*[i,j+1] = R[i,j]
```

Hệ số:

```text
lower[k]    = -μ
diagonal[k] = 1 + 2μ
upper[k]    = -μ
lower[0] = 0, upper[interior_size-1] = 0
```

### Y-sweep

Với mỗi cột interior, giải hệ tam đường chéo theo hàng (theo y):

```text
-μ uⁿ⁺¹[i-1,j] + (1+2μ) uⁿ⁺¹[i,j] - μ uⁿ⁺¹[i+1,j] = u*[i,j]
```

Hệ số giống X-sweep. RHS là giá trị `u_half` tại cột tương ứng.

## Bộ giải hệ tam đường chéo

- Thomas algorithm (`solver=thomas`)
- Cyclic reduction (`solver=cr`)

Quy ước vector: tất cả `lower`, `diagonal`, `upper`, `rhs` có cùng độ dài `n`. `lower[0] = 0`, `upper[n-1] = 0`.

## Kiểm tra

So sánh với nghiệm chính xác analític (hàm `exact_solution` trong `wave_problem.cpp`):

```text
u(t,x,y) = sin(πx/L) sin(πy/L) cos(√2 π c t / L)
```
