## 4. 数据并行与 SIMD

> **单指令多数据** — 拓宽位宽，一条指令处理多个元素

---

### 一、SIMD 基础

| 寄存器 | 指令集 | 宽度 |
|--------|--------|------|
| **XMM** | SSE | **128 bit** |
| **YMM** | AVX | **256 bit** |
| **ZMM** | AVX-512 | 512 bit（新平台） |

即使 **单条指令** 不再拆分并发，仍可通过 **数据并行** 提高 **有效吞吐**。

→ [02-Hennessy Ch4 SIMD/GPU](../../../02-Computer-Architecture-6th/chapter-04-vector-simd-gpu/) · [Ch1 SIMD 提及](../chapter-01-认识DPDK/notes/section-3-性能最佳实践.md)

---

### 二、I/O 密集负载的收益

DPDK 类负载：**访存带宽** 常是瓶颈。

SIMD 的 **直接好处：**

- **最大化 L1 带宽** — 宽 Load/Store 一次搬更多字节  
- 减少流水线因 **等待内存** 而 **stall**

---

### 三、实战：`rte_memcpy`

DPDK **放弃**  libc `memcpy`，专用 **`rte_memcpy`**：

| 技巧 | 说明 |
|------|------|
| **最宽 Load/Store** | 平台允许的最大宽度（如 Haswell **256b AVX**） |
| **Store 地址对齐** | 优先保证 **写** 对齐 |
| **双 Load / 周期** | 利用超标量 **每周期两条 Load** — 弥补 **非对齐 Load** 损失 |

**场景：** mbuf 拷贝、批量搬包头 — 微优化在 **每包 copy** 路径上累积显著。

> **深潜：** `lib/librte_eal/*/rte_memcpy` — 平台分派（SSE/AVX/NEON）。

**HFT：** 热路径 **避免 memcpy** 优于 **更快 memcpy** — 零拷贝、指针传递优先。

---

← [3. ILP](./section-3-指令级并发.md) · 下一节 [5. 小结](./section-5-小结与索引.md)
