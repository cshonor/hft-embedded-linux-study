## 4. CPU 与 I/O 协奏优化

> 包在 **CPU ↔ 内存 ↔ 网卡** 间流转：大量 **普通访存** + **MMIO**（映射到 PCIe）

---

### 一、减少 MMIO 频度

**MMIO**（写 Tail 等寄存器）需走 **PCIe**，延迟 **远高于** 本地内存访问。

DPDK 策略：

| 手段 | 说明 |
|------|------|
| **批量 refill** | 描述符 **空置率** 到阈值再 **集中重填** + **一次** 更新 Tail |
| **批量 TX 完成确认** | 集中检查 DD / 写回标志，而非每包 touch 寄存器 |
| **burst RX/TX** | PMD `rte_eth_rx_burst` / `tx_burst` — 摊薄 MMIO |

**HFT：** 即使低 PPS，也应用 **burst API** 保持与 DPDK 模型一致，避免隐式 per-packet MMIO。

---

### 二、提高 PCIe 传输效率

- **合并多个描述符操作**，使单次 TLP 净荷尽量接近 **整 Cache Line**（通常 64B）  
- 提高 **带宽利用率**，减少 TLP 头部占比  

→ 与 [Ch3 SIMD/memcpy](../chapter-03-并行计算/notes/section-4-数据并行与SIMD.md) 同属「凑满总线宽度」思路。

---

### 三、避免 Cache Line 部分写

**部分写 (Partial Write)**：未对齐或未占满 Cache Line 的写 → 内存子系统 **读-改-写 (RMW)** 合并 → 额外延迟。

DPDK 对策：

- Buffer / mbuf 数据区 **严格 Cache Line 对齐**  
- 描述符环、控制结构 **对齐填充** — 衔接 [Ch2 Cache 对齐](../chapter-02-Cache与内存/notes/section-4-Cache一致性与无锁设计.md)

---

### 四、全景协奏

```
CPU 填描述符 (访存) → MMIO 更新 Tail (PCIe)
    → NIC DMA 读/写帧 (PCIe MRd/MWr)
    → CPU 读 mbuf 元数据 (L1/L2)
    → 处理 → 再入 TX 环
```

瓶颈可能在 **任一环节** — 用 perf + PCIe 带宽估算（§5）定位。

---

← [3. DMA 描述符环](./section-3-DMA描述符环形队列.md) · 下一节 [5. 净荷带宽计算](./section-5-PCIe净荷带宽计算.md)
