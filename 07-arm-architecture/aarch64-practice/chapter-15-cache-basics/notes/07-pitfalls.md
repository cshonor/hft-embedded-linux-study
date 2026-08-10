# §15.7 易错点清单

> **来源：** [Ch15 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

Cache 的常见错误总结：DMA 忘做 Cache 操作、混淆 Clean 和 Invalidate、自修改代码忘清 I-Cache、VIPT 别名、伪共享未对齐。每个错误都有明确的症状和修复方法。

## 核心要点

### 6 大易错点

| # | 易错点 | 后果 | 症状 | 修复 |
|---|--------|------|------|------|
| 1 | DMA 忘做 Cache 操作 | 设备读到旧数据，或 CPU 读到旧数据 | DMA 数据偶尔错误 | DMA 前 invalidate/clean |
| 2 | 混淆 Clean 和 Invalidate | Clean 只写回不丢弃；Invalidate 只丢弃不写回 | 数据不更新或数据丢失 | 理解操作语义 |
| 3 | 自修改代码忘清 I-Cache | CPU 执行旧的指令缓存 | 代码修改不生效 | clean D-cache → invalidate I-cache |
| 4 | VIPT 别名 | 多个 VA 映射同 PA 时 Cache 不一致 | 随机数据不一致 | 内核通过页着色避免 |
| 5 | 伪共享 | 不同核变量在同一 cache line | 多核性能暴跌 | 对齐填充到 64 字节 |
| 6 | 硬编码 cache line 大小 | 不同平台 cache line 不同 | cache 操作对齐错误 | 从 CTR_EL0 动态读取 |

### 调试技巧表

| 症状 | 可能原因 | 检查方法 |
|------|----------|----------|
| DMA 数据偶尔错误 | 忘 invalidate/clean cache | 检查 DMA 前后是否有 cache 维护 |
| 修改代码后仍执行旧指令 | I-Cache 缓存旧指令 | 加 clean D-cache + invalidate I-cache |
| 多 VA 映射同 PA 数据不一致 | VIPT 别名 | 避免手动创建多 VA→同 PA 映射 |
| 多核性能远低于预期 | 伪共享 | 用 perf c2c 检查 HITM |
| 性能异常低 | cache line 冲突 | 检查数据结构大小是否是组大小倍数 |
| 不同平台 cache 行为不同 | 硬编码 cache line = 64 | 用 CTR_EL0 动态获取 |

### 常见代码错误对比

```c
// 错误 1: DMA 忘 invalidate
void dma_receive_wrong(void *buf, size_t size) {
    start_dma(buf, size);  // DMA 写内存
    wait_dma_complete();
    // 忘记 invalidate → CPU 读到 cache 中的旧数据
    process_data(buf);    // 可能读到旧值！
}

// 正确：先 invalidate 再 DMA
void dma_receive_correct(void *buf, size_t size) {
    cache_invalidate(buf, size);  // 先 invalidate
    start_dma(buf, size);         // DMA 写内存
    wait_dma_complete();
    process_data(buf);            // cache miss → 从内存读新数据
}

// 错误 2: 该 Invalidate 时用了 Clean
void dma_after_receive_wrong(void *buf, size_t size) {
    // DMA 已写入新数据，CPU cache 有旧数据
    cache_clean(buf, size);  // 错！Clean 只写回不丢弃
    // → cache 行仍在 → CPU 仍读 cache 中的旧值
    process_data(buf);  // 读到旧值！
}

// 正确：用 Invalidate
void dma_after_receive_correct(void *buf, size_t size) {
    cache_invalidate(buf, size);  // 丢弃旧 cache 行
    process_data(buf);  // cache miss → 从内存读新数据
}

// 错误 3: 自修改代码只 invalidate I-cache
void self_modify_wrong(void *code, uint32_t new_insn) {
    *(uint32_t *)code = new_insn;  // 写新指令到 D-cache
    ic_ivau(code);                 // 只 invalidate I-cache
    // → 新指令还在 D-cache 没写回内存 → I-cache miss 从内存读旧指令
}

// 正确：先 clean D-cache 再 invalidate I-cache
void self_modify_correct(void *code, uint32_t new_insn) {
    *(uint32_t *)code = new_insn;  // 写新指令（在 D-cache 中）
    dc_cvau(code);                 // clean D-cache（写回内存）
    dsb sy;                        // 等写回完成
    ic_ivau(code);                 // invalidate I-cache（丢弃旧指令）
    dsb sy;                        // 等 invalidate 完成
    isb;                           // 冲刷流水线
}
```

## HFT 关联

DMA cache 操作错误在 HFT 系统中是最常见的隐蔽 bug——数据"偶尔"不对，因为 cache 命中时读到旧值，miss 时读到新值，行为不确定。HFT 系统如果用 DMA 收发网络包，必须确保每次 DMA 操作都有正确的 cache 维护。自修改代码在 HFT 中不常见（通常不用 JIT），但如果用 eBPF 或动态补丁，必须处理 I-Cache 一致性。

HFT 代码审查必查项：1) DMA buffer 前后是否有 cache 维护；2) 多核共享变量是否对齐到 cache line；3) 自修改代码是否做了 D-cache clean + I-cache invalidate。

## 自测题

1. **DMA 从设备读数据后 CPU 读到旧值，最可能的原因是什么？**

<details>
<summary>答案</summary>

**忘记 invalidate CPU cache**。DMA 写入新数据到内存，但 CPU cache 中还有旧数据。CPU 读时命中 cache → 读到旧值。修复：DMA 写之前 `dc ivac`（invalidate）对应区域，强制 CPU 下次从内存读。
</details>

2. **自修改代码修改后 CPU 仍执行旧指令，应该怎么修复？**

<details>
<summary>答案</summary>

1. **Clean D-cache**：新指令写在 D-cache 中，需写回内存（`dc cvac` 或 `dc cvau`）
2. **DSB**：等写回完成
3. **Invalidate I-cache**：丢弃 I-cache 中的旧指令缓存（`ic ivau`）
4. **DSB + ISB**：确保后续取指用新指令

顺序很重要：先写回 D-cache，再清 I-cache。因为 I-cache 和 D-cache 是分离的。

注意：如果 CTR_EL0 的 DIC=1 且 IDC=1（如 A76），硬件自动维护，不需要手动操作。
</details>

3. **Clean 和 Invalidate 误用会有什么后果？**

<details>
<summary>答案</summary>

- 该 **Invalidate** 时用了 **Clean**：cache 行仍有效，CPU 仍读旧值（DMA 场景）
- 该 **Clean** 时用了 **Invalidate**：脏数据丢失！CPU 写的新数据被丢弃，没有写回内存 → DMA 读到旧值或内存数据丢失

两种误用都会导致数据不一致，但症状不同。Clean 误用通常"数据不更新"，Invalidate 误用通常"数据丢失"。
</details>

4. **为什么不应硬编码 cache line 大小为 64 字节？**

<details>
<summary>答案</summary>

不同 ARM 核的 cache line 大小可能不同——A72/A76 的 L1 D-cache line 是 64 字节，但 L3 可能是 128 字节，其他平台可能不同。硬编码 64 会导致 cache 维护操作不对齐（漏处理部分 cache line 或多处理），可能遗漏脏数据或引入虚假 invalidate。

正确做法：从 `CTR_EL0` 的 DminLine 字段动态计算 `cache_line_size = 4 << DminLine`，确保跨平台兼容。
</details>

## 参考与延伸

- [§15.5 DMA 与 Cache](05-dma-cache.md) — DMA cache 操作详解
- [§15.4 关键概念](04-key-concepts.md) — Clean/Invalidate/Flush 语义
- [Ch16 §16.4 自修改代码](../../chapter-16-cache-coherency/notes/04-self-modifying-code.md) — I-Cache 一致性
- [Ch16 §16.2 伪共享](../../chapter-16-cache-coherency/notes/02-false-sharing.md) — 伪共享修复
