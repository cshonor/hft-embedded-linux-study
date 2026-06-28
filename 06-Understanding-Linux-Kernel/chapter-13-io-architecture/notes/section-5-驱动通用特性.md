## 5. 设备驱动程序的通用特性

---

### 一、监控 I/O 完成：轮询 vs 中断

硬件操作 **耗时不确定**，驱动需知道何时完成：

| 模式 | 行为 | 适用 |
|------|------|------|
| **轮询 (Polling)** | 循环读 **状态寄存器** 直到完成 | 极快设备、或可接受占 CPU |
| **中断 (Interrupts)** | 设备完成后发 **IRQ** — CPU 做别的事，中断时再处理 | **主流** — 更高效 |

→ IRQ 框架：[Ch 4 section-6](../chapter-04-interrupts-and-exceptions/notes/section-6-IO中断处理.md) · [Ch 4 section-8 返回路径](../chapter-04-interrupts-and-exceptions/notes/section-8-中断返回.md)

HFT：高 PPS 网卡常 **NAPI / 批量 poll** — 在中断与纯轮询之间折中（modern，ULK 2.6 已有中断主导模型）。

---

### 二、直接内存访问 (DMA)

大块数据传输时，让 **设备控制器** 直接与 **RAM** 交换数据，减轻 CPU **逐字节拷贝**。

内核提供 **架构无关** 的 DMA 辅助 API，并区分两类映射：

| 类型 | 用途 |
|------|------|
| **一致性 DMA (Coherent)** | CPU 与设备 **共享** 同一份内存视图 — 适合小控制结构；处理 **缓存一致性** |
| **流式 DMA (Streaming)** | 单次传输 **映射/解除** — 适合 **scatter-gather** 大数据流；方向明确（to/from device） |

→ ZONE_DMA 低端内存：[Ch 8](../chapter-08-memory-management/notes/section-2-页框管理.md) — 部分老设备 DMA 只能寻址低 16MB。

> **深潜可选：** `dma_alloc_coherent` vs `dma_map_single` — 总线是否 I/O coherent 决定是否需要 bounce buffer。

---

← [4. 设备文件](./section-4-设备文件.md) · 下一节 [6. 字符设备](./section-6-字符设备驱动.md)
