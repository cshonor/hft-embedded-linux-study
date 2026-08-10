# §19.3 案例三：DMA 操作

> **来源：** [Ch19 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

DMA 场景必须用 DSB 而不是 DMB：DMB 不停 CPU，`start_dma()` 可能先执行；DSB 完全停住 CPU 确保 `prepare_tx_buffer()` 写入完成后才执行 `start_dma()`。

## 核心要点

### DMA 屏障模式

```c
// CPU 准备数据给 DMA 读取
prepare_tx_buffer(buf, len);
// 必须用 DSB（不是 DMB！）
mb();  // = dsb sy，确保数据完全写入内存（对 DMA 可见）
start_dma(DMA_TX, buf, len);

// DMA 写完后 CPU 读取
wait_dma_complete();
rmb();  // = dsb ld，确保 DMA 写入对 CPU 可见
process_rx_buffer(buf, len);
```

### 为什么用 DSB 而不是 DMB？

| 屏障 | 行为 | DMA 场景问题 |
|------|------|-------------|
| DMB | 访存有序，CPU 不停 | `start_dma()` 可能先执行 → DMA 读到不完整数据 |
| DSB | 完全停住，等访存完成 | 确保 `prepare_tx_buffer()` 写入完成后才执行 `start_dma()` |

> DMA 场景必须用 DSB，不能用 DMB。这是 Ch18 的知识点在实战中的应用。

### CPU↔DMA vs CPU↔CPU

| 场景 | 屏障 | 原因 |
|------|------|------|
| CPU ↔ CPU | `dmb ish` (DMB) | CPU 间只需顺序保证 |
| CPU ↔ DMA | `dsb sy` (DSB) | DMA 需要完全完成，DSB 停住 CPU |

## HFT 关联

DMA 是 HFT 网络收发包的核心机制。如果用 DMA 发送订单数据，必须用 DSB 确保数据写入内存后才启动 DMA——否则 DMA 可能读到不完整的订单数据，发送错误的交易指令。在 DPDK 中，`rte_wmb()` 通常展开为 DSB（而非 DMB），因为 DPDK 的网络包发送涉及 DMA。在 Pi5 裸金属 HFT 中，手动写 `dsb sy` 确保网卡门铃寄存器写入后才触发 DMA 发送。

## 自测题

1. **DMA 场景为什么必须用 DSB 而不是 DMB？**

<details>
<summary>答案</summary>

DMB 只保证访存顺序但 **CPU 不停**——`start_dma()` 可能在 `prepare_tx_buffer()` 的数据还没完全写入内存时就执行（因为 DMB 不阻止后续非访存指令或不同地址的访存）。DSB **完全停住 CPU**，确保 `prepare_tx_buffer()` 的所有写入完成后才执行 `start_dma()`。DMA 需要的是"完成"而非"顺序"，必须用 DSB。
</details>

2. **CPU↔CPU 和 CPU↔DMA 分别用什么屏障？为什么不同？**

<details>
<summary>答案</summary>

- **CPU↔CPU**：`dmb ish`（DMB）。CPU 间只需访存顺序保证（Store-Store 不重排等），DMB 足够。CPU 可以继续执行非访存指令。
- **CPU↔DMA**：`dsb sy`（DSB）。DMA 是外部设备，不参与 CPU 的 cache 一致性协议。需要 DSB 完全停住 CPU，确保数据写入内存（对 DMA 可见）后才启动 DMA 操作。

DMB 对 DMA 不够强，因为 DMB 不保证数据到达 DMA 可观察的点（PoC）。
</details>

3. **`mb()` 和 `smp_mb()` 在 DMA 场景中应该选哪个？**

<details>
<summary>答案</summary>

DMA 场景选 **`mb()`**（= `dsb sy`）。`smp_mb()`（= `dmb ish`）不够强——DMB 不保证数据对 DMA 可见。`mb()` 用 DSB 完全停住 CPU，确保数据写入到 PoC（Point of Coherency，DMA 可观察的点）后才继续。这是 `smp_*` 和非 `smp_*` API 的关键区别：`smp_*` 只管 CPU 间，非 `smp_*` 管 CPU↔DMA。
</details>

## 参考与延伸

- [§19.5 屏障选择决策树](05-decision-tree.md) — DMA 在决策树中的位置
- [Ch18 §18.2 三条屏障指令](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md) — DMB vs DSB 详解
- [Ch16 §16.3 DMA 一致性](../../chapter-16-cache-coherency/notes/section-0-本章完整概述.md) — DMA cache 操作
