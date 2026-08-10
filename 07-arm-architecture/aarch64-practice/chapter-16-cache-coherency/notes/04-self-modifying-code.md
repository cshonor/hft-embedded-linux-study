# §16.4 自修改代码

> **来源：** [Ch16 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

修改代码后必须刷新 I-Cache：先 clean D-cache（写回新指令），再 invalidate I-cache（丢弃旧指令缓存）。本节分析自修改代码的 cache 一致性原理、flush_icache_range 的完整实现、应用场景和常见 bug。

## 核心要点

### 自修改代码的 cache 一致性

```
CPU 写新指令
  → 数据进入 D-cache（Write-Back 模式，内存尚未更新）
  → I-cache 仍缓存旧指令（I/D 分离！）

必须执行：
  1. Clean D-cache → 新指令从 D-cache 写回内存
  2. DSB → 等待写回完成
  3. Invalidate I-cache → 丢弃旧指令缓存
  4. DSB → 等待 invalidate 完成（所有核）
  5. ISB → 冲刷流水线，确保重新取指
```

### 完整 flush_icache_range 实现

```c
// 裸金属版本
void flush_icache_range(unsigned long start, unsigned long end) {
    unsigned long addr = start & ~(CACHE_LINE_SIZE - 1);
    
    // Step 1: Clean D-cache（写回新指令到内存）
    while (addr < end) {
        asm volatile("dc cvau, %0" :: "r"(addr));
        addr += CACHE_LINE_SIZE;
    }
    
    // Step 2: DSB（确保 D-cache clean 完成）
    asm volatile("dsb sy" ::: "memory");
    
    // Step 3: Invalidate I-cache（丢弃旧指令）
    addr = start & ~(CACHE_LINE_SIZE - 1);
    while (addr < end) {
        asm volatile("ic ivau, %0" :: "r"(addr));
        addr += CACHE_LINE_SIZE;
    }
    
    // Step 4: DSB（确保 I-cache invalidate 在所有核完成）
    asm volatile("dsb sy" ::: "memory");
    
    // Step 5: ISB（冲刷流水线，重新取指）
    asm volatile("isb" ::: "memory");
}
```

### 为什么需要两步？

| 步骤 | 原因 | 漏掉后果 |
|------|------|----------|
| Clean D-cache | 新指令写在 D-cache 中（CPU 写是 D-cache 操作），需写回内存 | CPU 从内存读到旧指令 |
| Invalidate I-cache | I-cache 缓存了旧指令，不 invalidate 则 CPU 执行旧指令 | CPU 执行旧版本代码 |
| DSB（中间） | 确保 clean 完成后再 invalidate | invalidate 可能先于 clean 完成 |
| DSB（末尾） | 确保 I-cache invalidate 在所有核完成 | 其他核仍执行旧指令 |
| ISB | 冲刷流水线中预取的旧指令 | 流水线中有旧指令继续执行 |

> I-Cache 和 D-Cache 是分离的！CPU 写代码写到 D-cache，取指从 I-cache 读。
> 必须先让新指令到内存（clean D-cache），再让 I-cache 重新加载（invalidate I-cache）。

### 应用场景

| 场景 | 说明 | cache 操作 |
|------|------|-----------|
| JIT 编译器 | 动态生成代码后执行 | clean D-cache + invalidate I-cache |
| 内核 module 加载 | 加载 .ko 文件中的代码 | flush_icache_range |
| kprobes/ftrace | 动态插桩（替换指令） | clean + invalidate |
| 热补丁 | 运行时替换函数 | clean + invalidate + ISB |
| eBPF JIT | BPF 程序编译为本地码 | flush_icache_range |

### 多核自修改代码

```c
// 多核场景：修改代码后需要让所有核的 I-cache 失效
void flush_icache_all_cpus(void *code_addr, size_t len) {
    // 1. 本核 clean D-cache + invalidate I-cache
    flush_icache_range((unsigned long)code_addr, 
                       (unsigned long)code_addr + len);
    
    // 2. 发送 IPI 让其他核也 invalidate I-cache
    smp_call_function(flush_icache_others, code_addr, len);
}

// Linux 内核用 __flush_icache_all() 刷所有核的 I-cache
// 底层用 ic ialluis（Invalidate ALL, Inner Shareable）
```

### ARMv8 cache 维护指令（自修改代码相关）

| 指令 | 作用 | PoP/PoU |
|------|------|---------|
| `dc cvau, x0` | Clean data cache to PoU | 写回 D-cache 到 unification point |
| `ic ivau, x0` | Invalidate I-cache to PoU | 失效 I-cache |
| `ic iallu` | Invalidate entire I-cache（本核） | 全刷本核 I-cache |
| `ic ialluis` | Invalidate entire I-cache（Inner Shareable） | 全刷所有核 I-cache |
| `dsb ish` | Data Sync Barrier（Inner Shareable） | 等待所有核完成 |
| `isb` | Instruction Sync Barrier | 冲刷流水线 |

> PoU（Point of Unification）：I-cache 和 D-cache 汇聚的点，通常在 L2。

## HFT 关联

HFT 系统通常不使用自修改代码（确定性要求高，JIT 不可控）。但如果用 eBPF 做运行时监控，或用动态补丁修复 bug，必须正确处理 I-Cache 一致性。

### 漏 invalidate I-cache 的隐蔽性

漏 invalidate I-cache 是最隐蔽的 bug——代码在开发机上"偶尔"不对：
- I-cache miss 时 → 从内存加载新指令 → 正确
- I-cache hit 时 → 执行旧指令 → 错误
- 是否 hit 取决于之前的执行历史 → 不可预测

ISB 指令在自修改代码后必须执行，确保流水线中没有旧指令。在 ARMv8 中，`ic ialluis` 可以一次刷新所有核的 I-cache（Inner Shareable），用于内核热补丁场景。

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

4. **JIT 编译器生成一段代码后，完整的 cache 维护步骤是什么？**

<details>
<summary>答案</summary>

```
1. 写入新指令到代码区域（D-cache 操作）
2. dc cvau（clean D-cache to PoU，写回新指令）
3. dsb sy（等待 clean 完成）
4. ic ivau（invalidate I-cache 对应区域）
5. dsb sy（等待 invalidate 在所有核完成）
6. isb（冲刷流水线，重新取指）
7. 可以安全跳转到新代码执行
```
</details>

## 参考与延伸

- [§16.3 DMA 一致性](03-dma-coherency.md) — 类似的 cache 一致性问题
- [§16.6 易错点](06-pitfalls.md) — 自修改代码的常见错误
- [Ch18 §18.2 三条屏障指令](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md) — DSB/ISB 详解
