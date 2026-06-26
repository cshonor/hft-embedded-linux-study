## 4.4 图形处理器 (GPU)

### CUDA 与 SIMT

**SIMT (Single Instruction, Multiple Thread)**：一条指令驱动 **一组线程** 并行（NVIDIA **Warp = 32 线程**）。

| 层次 | CUDA 术语 | 类比 |
|------|-----------|------|
| 网格 | **Grid** | 整个向量化问题 / 大循环 |
| 线程块 | **Thread Block** | 可协作的工作组 |
| 执行单元 | **SM (Streaming Multiprocessor)** | 多线程 SIMD 处理器 |

**调度：** 硬件将 warp **锁步** 执行同一条指令；用 **超多线程** 隐藏 DRAM 延迟。

---

### 内存层次

| 类型 | 范围 | 速度 |
|------|------|------|
| **Private** | 单线程寄存器/局部 | 最快 |
| **Local / Shared** | 同 **Block** 内共享 | 片上，快 |
| **Global** | 全 Grid | 大但慢（DRAM） |

**地址合并 (Address Coalescing)**：warp 内线程访问 **相邻地址** → 合并为少量事务 → **带宽利用率高**；**散乱访问** 则效率暴跌。

---

### 分支发散 (Branch Divergence)

`if-else` 时 warp 内线程走不同路径：

1. 硬件 **先执行 THEN 路径**（掩码屏蔽不满足线程）
2. 再执行 **ELSE 路径**
3. 有效吞吐 ≈ **单路径** — 发散严重时 GPU 优势消失

| HFT 视角 |
|----------|
| **GPU 适用**：离线回测、蒙特卡洛、ML 训练/批量推理、大规模风险仿真 |
| **不适用**：交易所 colo **tick-to-trade** — 延迟、确定性、PCIe 往返不可接受 |
| 若用 GPU 做研究：注意 **合并访存**、少发散分支 — 与 Ch2 局部性同构 |
| SmartNIC/FPGA 是另一路线（→ [Ch7 DSA](../../chapter-07-特定领域架构.md)、[14-DPDK](../../../14-DPDK-Low-Latency-Network/)） |

---
