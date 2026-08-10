# §16.6 易错点清单

> **来源：** [Ch16 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

Cache 一致性的 6 个常见错误：伪共享、DMA 不做 Cache 操作、自修改代码不刷 I-Cache、MESI 状态记混、该 invalidate 用了 clean、硬编码 cache line 大小。

## 核心要点

| # | 易错点 | 后果 | 修复 |
|---|--------|------|------|
| 1 | 伪共享 | 不同核的变量在同一 Cache Line，反复 invalidate | 对齐填充到 64 字节 |
| 2 | DMA 不做 Cache 操作 | 数据不一致 | DMA 前 invalidate/clean |
| 3 | 自修改代码不刷 I-Cache | 执行旧指令 | clean D-cache → invalidate I-cache |
| 4 | MESI 记混 | M=修改（内存过期）；E=独占（=内存）；S=共享（=内存，多核有副本） | 理解状态语义 |
| 5 | 该 invalidate 用了 clean | clean 只写回不丢弃 → CPU 仍读旧值 | DMA 接收用 invalidate 不是 clean |
| 6 | 硬编码 cache line 大小 | 不同 CPU cache line 不同 → padding 错误 | 用 `sysconf` 或 `CTR_EL0` 动态获取 |

### MESI 速记

| 状态 | 一句话记忆 | 写操作 | evict 行为 |
|------|-----------|--------|-----------|
| M | 改了，只有我有，内存过期 | 0 cycle（直接写） | 必须写回内存 |
| E | 只有我有，内存是最新的 | 0 cycle（M→E 不变）→ 实际变 M | 直接丢弃 |
| S | 大家都有，都是最新的 | 需要 invalidate 其他核 → 50-100ns | 直接丢弃 |
| I | 我的副本无效 | 需要重新加载 | 无数据 |

### 常见代码错误对比

| # | 错误代码 | 正确代码 | 问题 |
|---|---------|---------|------|
| 1 | `dma_cache_clean(buf, len); dma_rx(buf);` | `dma_cache_invalidate(buf, len); dma_rx(buf);` | DMA 接收应用 invalidate 不是 clean |
| 2 | `dc cvac, addr` 后直接读 | `dc ivac, addr` 后读 | clean 不丢弃 cache 行，CPU 仍读旧值 |
| 3 | 改代码后 `ic ivau` 跳过 `dc cvau` | `dc cvau` → `dsb` → `ic ivau` → `dsb` → `isb` | 漏 clean D-cache 导致内存还是旧指令 |

### 调试技巧

| 症状 | 检查方向 | 工具 |
|------|----------|------|
| 多核性能远低于预期 | 伪共享 | `perf c2c` |
| DMA 数据偶尔错误 | Cache 一致性（invalidate/clean） | 在 DMA 前后加 cache 操作 |
| 代码修改不生效 | I-Cache（invalidate） | 加 `flush_icache_range` |
| 共享变量写后其他核看不到 | 内存屏障（DMB） | 加 `dmb ishst` |
| DMA 接收前几字节正确后面错误 | cache line 对齐问题 | 确保 invalidate 覆盖全部 buffer |
| 不同 CPU 上行为不同 | 硬编码 cache line | 用 `sysconf` 动态获取 |

### Cache 操作选择速查

| 场景 | 正确操作 | ARMv8 指令 | 常见错误 |
|------|---------|-----------|---------|
| DMA 接收前 | invalidate | `dc ivac` | 用了 clean（不丢弃旧值） |
| DMA 发送前 | clean | `dc cvac` | 用了 invalidate（丢弃了新值！） |
| 自修改代码 | clean D + inval I | `dc cvau` + `ic ivau` | 只做其中一步 |
| 页表修改后 | invalidate | `tlbi vae1` | 忘了刷 TLB |
| 新映射建立后 | 无需操作 | — | 多余的 flush |

> **关键区别**：clean = 写回内存但**保留** cache 副本；invalidate = **丢弃** cache 副本不写回。
> DMA 接收要丢弃旧副本（invalidate）；DMA 发送要写回新副本（clean）。

## HFT 关联

这 6 个错误在 HFT 系统中都有致命后果。伪共享导致延迟抖动（不可预测的 cache line 传输）。DMA cache 错误导致网络数据丢失或损坏。自修改代码 I-Cache 问题导致"偶发"执行旧逻辑。MESI 理解不足导致错误使用共享变量。该 invalidate 用了 clean 导致 DMA 接收读到旧数据。硬编码 cache line 在不同 CPU 上 padding 错误。

HFT 开发者应该把这 6 个检查点作为代码审查的必查项。

### HFT 代码审查清单

```c
// ✓ 检查项 1：每核变量是否对齐
struct alignas(64) per_cpu_data { ... };  // 正确

// ✓ 检查项 2：DMA 接收前是否 invalidate
dma_cache_invalidate(rx_buf, len);  // 正确

// ✓ 检查项 3：DMA 发送前是否 clean
dma_cache_clean(tx_buf, len);  // 正确

// ✓ 检查项 4：cache line 大小是否动态获取
int clsize = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);  // 正确
// 而非 #define CACHE_LINE 64  // 硬编码

// ✓ 检查项 5：自修改代码是否完整 flush
// dc cvau → dsb → ic ivau → dsb → isb  // 完整序列

// ✓ 检查项 6：共享变量是否有屏障
// dmb ishst（Store-Store 屏障）  // 确保可见性
```

## 自测题

1. **MESI 四个状态中，哪个状态的 cache 行被 evict 时不需要写回内存？**

<details>
<summary>答案</summary>

**E**（Exclusive）和 **S**（Shared）。因为 E 和 S 状态的 cache 行与内存一致（Cache = 内存），evict 时直接丢弃即可。**M**（Modified）状态与内存不一致（内存过期），evict 时必须写回内存。**I**（Invalid）是无效的，没有数据需要处理。
</details>

2. **多核性能异常低，应该首先检查什么？**

<details>
<summary>答案</summary>

首先检查**伪共享**。用 `perf c2c record/report` 查看 HITM 指标。如果 HITM 高，说明 cache line 在核间频繁传输，很可能有伪共享。然后检查多核共享变量的布局，用 `aligned(64)` 修复。
</details>

3. **共享变量写后其他核看不到新值，可能的原因有哪些？**

<details>
<summary>答案</summary>

可能原因：
1. **缺少内存屏障**：ARM 弱序模型，Store-Store 可能重排，需要 `dmb ishst` 或 `smp_wmb()`
2. **编译器重排**：硬件屏障不阻止编译器重排，需要 `barrier()` 或 `volatile`
3. **Cache 一致性问题**（较少见）：MESI 应自动处理，但如果 DMA 涉及可能需要手动 flush
4. **`atomic_read` 不保证可见性**：需要 `smp_mb__after_atomic()` 或使用 acquire/release 语义
</details>

4. **DMA 接收数据前应该用 clean 还是 invalidate？为什么不能用另一个？**

<details>
<summary>答案</summary>

用 **invalidate**（`dc ivac`），不是 clean。

- **invalidate**：丢弃 cache 中的旧副本，不写回内存。CPU 下次读时 cache miss → 从内存读到 DMA 写入的新值。正确。
- **clean**（错误）：将 cache 中的脏数据写回内存，但**保留** cache 副本。CPU 下次读时 cache hit → 读到**旧值**（DMA 写入内存的新值被 cache 覆盖）。

clean 反而会把旧数据写回内存，覆盖 DMA 写入的新数据，比不做 cache 操作更糟糕。
</details>

## 参考与延伸

- [§16.1 MESI 协议](01-mesi.md) — MESI 状态详解
- [§16.2 伪共享](02-false-sharing.md) — 伪共享修复
- [Ch18 §18.7 易错点](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md) — 内存屏障相关错误
