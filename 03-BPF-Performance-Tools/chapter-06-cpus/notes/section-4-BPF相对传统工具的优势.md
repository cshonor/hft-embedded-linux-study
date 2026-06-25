# 4. BPF 相对传统工具的优势

| 盲区 | BPF 如何补 |
|------|------------|
| **极短命进程** | `top` 采样不到 → `execsnoop` |
| **运行队列等待** | `mpstat` 只见忙闲 → **`runqlat` 直方图** |
| **Off-CPU 原因** | `perf` 默认 on-CPU → **`offcputime`** |
| **按进程 LLC** | `perf stat` 粗粒度 → **`llcstat`** |

---
