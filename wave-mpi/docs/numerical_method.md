# Phương pháp số

## Bài toán

Phương trình sóng 2D trên miền vuông `[0, L] × [0, L]`:

```text
∂²u/∂t² = c² (∂²u/∂x² + ∂²u/∂y²)
```

## Điều kiện

- Ban đầu: `u(0,x,y) = sin(πx/L) sin(πy/L)`, `∂u/∂t(0,x,y) = 0`
- Biên Dirichlet: `u = 0` trên toàn bộ biên

## ADI (Alternating Direction Implicit)

Mỗi bước thời gian gồm:

1. **X-sweep:** giải hệ tam đường chéo theo từng hàng → `u_half`
2. **MPI_Allgatherv:** ghép `u_half` toàn cục
3. **Y-sweep:** giải hệ tam đường chéo theo từng cột → `u_next`
4. **MPI_Allgatherv:** ghép `u_next` toàn cục
5. Cập nhật `(u_previous, u_current)` cho bước tiếp theo

## Bộ giải hệ tam đường chéo

- Thomas algorithm (`solver=thomas`)
- Cyclic reduction (`solver=cr`)

## Kiểm tra

So sánh với nghiệm chính xác analític (hàm `exact_solution` trong `wave_problem.cpp`).
