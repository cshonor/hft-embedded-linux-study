## 3. 网卡 I/O 性能深度优化

> 收发包软件路径上的 **微架构级** 手段 — 与 [Ch3 ILP](../chapter-03-parallel-computing/notes/section-3-指令级并发.md)、[Ch6 MMIO 批量](../chapter-06-pcie-packet-io/notes/section-4-CPU与IO协奏优化.md) 一脉相承

---

### 一、Burst 收发包

| 要点 | 说明 |
|------|------|
| **API** | `rte_eth_rx_burst` / `rte_eth_tx_burst` — 一次处理 **8 / 16 / 32** 包 |
| **Cache** | 连续描述符/mbuf 访问 → **预取友好**，贴合 **64B Cache Line** |
| **MMIO** | 摊薄 **Tail 寄存器** 写频率（Ch6） |

**原则：** 即使业务低 PPS，仍用 **burst 接口** 保持 PMD 预期行为。

---

### 二、时延隐藏与批量处理

内存读、描述符 fetch **高延迟** — 利用 **超标量 + 乱序执行**：

- 将 **无数据依赖** 的多包处理 **铺开**（multi-packet pipeline）  
- 当前包等内存时，CPU 执行 **下一包** 独立指令  

→ [Ch3 Gustafson / 并行](../chapter-03-parallel-computing/) · PMD 内 **prefetch mbuf/描述符**

---

### 三、减少 Cache Line 冲突

**问题：** CPU **更新环尾 / 重填描述符** 与 NIC **DMA 写回** 争用 **同一 Cache Line**（读-写、写-写 bouncing）。

**对策：**

| 手段 | 效果 |
|------|------|
| **批量分配** 新 mbuf / 描述符 | 少次 touch 控制行 |
| **延迟更新 Tail** | 按 **Cache Line 整数倍** 移动尾指针 |
| **对齐** | 环、控制块 [Ch2/Ch6 对齐](../chapter-06-pcie-packet-io/notes/section-4-CPU与IO协奏优化.md) |

---

### 四、SIMD 向量化描述符

- **SSE Shuffle** 等 — **一次处理多个描述符** 字段转换  
- 与 [Ch3 SIMD / rte_memcpy](../chapter-03-parallel-computing/notes/section-4-数据并行与SIMD.md) 同思路 — **拓宽每周期工作量**

---

← [2. 轮询与混合中断](./section-2-轮询与混合中断模式.md) · 下一节 [4. 平台优化](./section-4-平台优化与配置调优.md)
