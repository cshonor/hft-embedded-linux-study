# §16.4 自修改代码

> **来源：** [Ch16 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

修改代码后必须刷新 I-Cache：先 clean D-cache（写回新指令），再 invalidate I-cache（丢弃旧指令缓存）。JIT 编译器、内核 module 加载、kprobes 都需要处理。

## 核心要点

### 自修改代码的 cache 一致性

```c
// 修改代码后必须刷新 I-Cache
void flush_icache_range(unsigned long start, unsigned long end) {
    // 1. Clean D-cache（写回修改的指令）
    // 2. DSB（确保写回完成）
    // 3. Invalidate I-cache（丢弃旧指令缓存）
    // 4. DSB + ISB（确保后续取指用新指令）
}
```

### 为什么需要两步？

| 步骤 | 原因 |
|------|------|
| Clean D-cache | 新指令写在 D-cache 中（CPU 写是 D-cache 操作），需写回内存 |
| Invalidate I-cache | I-cache 缓存了旧指令，不 invalidate 则 CPU 执行旧指令 |

> I-Cache 和 D-Cache 是分离的！CPU 写代码写到 D-cache，取指从 I-cache 读。
> 必须先让新指令到内存（clean D-cache），再让 I-cache 重新加载（invalidate I-cache）。

### 应用场景

| 场景 | 说明 |
|------|------|
| JIT 编译器 | 动态生成代码后执行 |
| 内核 module 加载 | 加载 .ko 文件中的代码 |
| kprobes/ftrace | 动态插桩 |
| 热补丁 | 运行时替换函数 |

## HFT 关联

HFT 系统通常不使用自修改代码（确定性要求高，JIT 不可控）。但如果用 eBPF 做运行时监控，或用动态补丁修复 bug，必须正确处理 I-Cache 一致性。漏 invalidate I-cache 是最隐蔽的 bug——代码在开发机上"偶尔"不对，因为 I-cache 有时命中旧指令有时 miss 加载新指令。ISB 指令在自修改代码后必须执行，确保流水线中没有旧指令。

## 自测题

1. **自修改代码为什么需要先 clean D-cache 再 invalidate I-cache？顺序能反吗？**

<details>
<summary>答案</summary>

CPU 写新指令到内存时，数据先到 D-cache（Write-Back）。必须先 clean D-cache 让新指令到内存，然后 invalidate I-cache 让下次取指从内存加载。**顺序不能反**：如果先 invalidate I-cache 再 clean D-cache，在两者之间如果发生取指，I-cache miss 从内存读到旧指令（新指令还在 D-cache 没写回）。
</details>

2. **修改代码后只 invalidate I-cache 不 clean D-cache，会发生什么？**

<details>
<summary>答案</summary>

I-cache invalidate 后，CPU 重新从内存取指。但新指令还在 D-cache 中没写回内存 → CPU 从内存读到**旧指令**。结果：修改代码"不生效"。这是自修改代码最常见的 bug——以为改了代码但 CPU 仍执行旧版本。
</details>

3. **flush_icache_range 最后的 DSB + ISB 分别做什么？**

<details>
<summary>答案</summary>

- **DSB**：等待 I-cache invalidate 在所有核上完成（确保旧指令缓存已清除）
- **ISB**：冲刷本核流水线，确保后续指令**重新从 I-cache/内存取指**（流水线中可能有旧指令）

不跟 DSB+ISB 的话，invalidate 可能还没完成就继续执行，或流水线中有旧指令。
</details>

## 参考与延伸

- [§16.3 DMA 一致性](03-dma-coherency.md) — 类似的 cache 一致性问题
- [§16.6 易错点](06-pitfalls.md) — 自修改代码的常见错误
- [Ch18 §18.2 三条屏障指令](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md) — DSB/ISB 详解
