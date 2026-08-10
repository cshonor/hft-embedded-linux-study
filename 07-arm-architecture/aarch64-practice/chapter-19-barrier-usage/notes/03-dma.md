# §19.3 案例三：DMA 操作

> **来源：** [Ch19 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

DMA 场景必须用 DSB 而不是 DMB：DMB 不停 CPU，`start_dma()` 可能先执行；DSB 完全停住 CPU 确保 `prepare_tx_buffer()` 写入完成后才执行 `start_dma()`。本节分析 DMA 发送/接收的屏障序列和 CPU↔CPU vs CPU↔DMA 的区别。

## 核心要点

### DMA 屏障模式

```c
// CPU 准备数据给 DMA 读取（内存→设备）
prepare_tx_buffer(buf, len);
// 必须用 DSB（不是 DMB！）
mb();  // = dsb sy，确保数据完全写入内存（对 DMA 可见）
start_dma(DMA_TX, buf, len);

// DMA 写完后 CPU 读取（设备→内存）
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

### DMA 发送完整序列

```
CPU 发送数据到设备：
1. prepare_tx_buffer(buf, len)   ← CPU 写数据到 D-cache
2. dc cvac, buf                  ← clean D-cache（写回内存）
3. dsb sy                        ← DSB：等 clean 完成 + 完全停住
4. start_dma(DMA_TX, buf, len)  ← 启动 DMA，从内存读数据
5. wait_dma_complete()           ← 等待 DMA 完成
```

### DMA 接收完整序列

```
设备发送数据到 CPU：
1. start_dma(DMA_RX, buf, len)  ← 启动 DMA，写数据到内存
2. wait_dma_complete()           ← 等待 DMA 完成
3. dsb sy                        ← DSB：确保 DMA 写入对 CPU 可见
4. dc ivac, buf                  ← invalidate D-cache（丢弃旧值）
5. dsb sy                        ← 等 invalidate 完成
6. process_rx_buffer(buf, len)   ← CPU 读数据
```

### CPU↔DMA vs CPU↔CPU

| 场景 | 屏障 | API | 展开为 | 原因 |
|------|------|-----|--------|------|
| CPU ↔ CPU | DMB | `smp_mb()` | `dmb ish` | CPU 间只需顺序保证 |
| CPU ↔ DMA | DSB | `mb()` | `dsb sy` | DMA 需要完全完成 |

### DMA API 对照

| API | 展开为 | 用途 |
|-----|--------|------|
| `mb()` | `dsb sy` | DMA 全屏障（Load+Store，全停住） |
| `rmb()` | `dsb ld` | DMA 读屏障 |
| `wmb()` | `dsb st` | DMA 写屏障 |
| `smp_mb()` | `dmb ish` | CPU 间全屏障（**不够 DMA！**） |

### DMA 方向与屏障

| DMA 方向 | 屏障 | cache 操作 | 何时 |
|----------|------|-----------|------|
| 内存→DMA（发送） | `wmb()` (dsb st) | clean | DMA 启动前 |
| DMA→内存（接收） | `rmb()` (dsb ld) | invalidate | DMA 完成后 |

## HFT 关联

DMA 是 HFT 网络收发包的核心机制。如果用 DMA 发送订单数据，必须用 DSB 确保数据写入内存后才启动 DMA——否则 DMA 可能读到不完整的订单数据，发送错误的交易指令。

### HFT 网卡发送序列

```c
// HFT 裸金属网卡发送（Pi5）
void nic_send(void *buf, size_t len) {
    // 1. 填充发送缓冲区
    memcpy(tx_buf, buf, len);
    
    // 2. clean D-cache（写回内存）
    dma_cache_clean(tx_buf, len);
    
    // 3. DSB：等 clean 完成且完全停住
    asm volatile("dsb sy" ::: "memory");
    
    // 4. 写网卡门铃寄存器（触发 DMA 发送）
    *(volatile uint32_t *)(NIC_BASE + NIC_TX_DOORBELL) = len;
    
    // 5. DSB：确保门铃写入完成
    asm volatile("dsb sy" ::: "memory");
}
```

在 DPDK 中，`rte_wmb()` 通常展开为 DSB（而非 DMB），因为 DPDK 的网络包发送涉及 DMA。在 Pi5 裸金属 HFT 中，手动写 `dsb sy` 确保网卡门铃寄存器写入后才触发 DMA 发送。

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

4. **DMA 接收（设备→内存）的完整屏障+cache 序列是什么？**

<details>
<summary>答案</summary>

```
1. wait_dma_complete()         ← 等 DMA 写完内存
2. dsb sy                      ← 确保 DMA 写入对 CPU 可见
3. dc ivac, buf                ← invalidate D-cache（丢弃旧值）
4. dsb sy                      ← 等 invalidate 完成
5. process_rx_buffer(buf, len) ← CPU 读到新值
```

先 DSB 等 DMA 完成，再 invalidate cache 让 CPU 读到内存中的新数据。不能只 invalidate 不 DSB——DMA 可能还没写完。
</details>

## 参考与延伸

- [§19.5 屏障选择决策树](05-decision-tree.md) — DMA 在决策树中的位置
- [Ch18 §18.2 三条屏障指令](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md) — DMB vs DSB 详解
- [Ch16 §16.3 DMA 一致性](../../chapter-16-cache-coherency/notes/section-0-本章完整概述.md) — DMA cache 操作
