# §16.5 实验要点

> **来源：** [Ch16 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

本章 2 个实验：高速缓存伪共享性能对比、使用 Perf C2C 发现伪共享。通过性能数据直观感受伪共享的影响。

## 核心要点

| 实验 | 内容 | 平台 | 关键技能 |
|------|------|------|----------|
| 16-1 | 高速缓存伪共享（性能对比） | Linux | 伪共享 vs 对齐的性能差异 |
| 16-2 | 使用 Perf C2C 发现伪共享 | Linux | perf c2c 工具使用 |

### 实验 16-1 性能对比

```c
// 有伪共享
struct { int a; int b; } data;  // a, b 在同一 cache line

// 无伪共享
struct { int a; char pad[60]; int b; } data;  // 不同 cache line

// 两个线程分别频繁写 a 和 b
// 对比两种情况的执行时间
```

### 实验 16-2 Perf C2C

```bash
# 采集 cache-to-cache 传输数据
perf c2c record ./program

# 查看报告
perf c2c report
# 关注 HITM（Hit In Modified）指标
```

## HFT 关联

实验 16-1 的性能对比数据对 HFT 开发者很有说服力——伪共享可以让性能下降 5-10 倍。HFT 系统中的每核统计计数器是最容易产生伪共享的地方。实验 16-2 的 perf c2c 工具是定位伪共享的利器，在生产环境中如果发现延迟抖动，可以用 perf c2c 检查是否有伪共享。在 Pi5 多核上，这个实验特别有意义——A76 的 L1/L2 私有，cache line 跨核传输开销显著。

## 自测题

1. **实验 16-1 中，伪共享版本比对齐版本慢多少？为什么？**

<details>
<summary>答案</summary>

伪共享版本通常比对齐版本慢 **5-10 倍**（具体取决于核数和写入频率）。原因：每次写操作都导致 cache line 在核间传输（MESI invalidate → reload），每次传输 ~50-100ns。对齐后每核独立 cache line，无跨核传输。
</details>

2. **Perf C2C 报告中哪个指标反映伪共享？**

<details>
<summary>答案</summary>

**HITM**（Hit In Modified）指标。HITM 高表示一个核频繁从另一个核的 M 状态 cache line 读取数据（跨核传输），是伪共享的强烈信号。报告还会显示具体地址和 cache line 偏移，帮助定位到具体变量。
</details>

3. **如何在 HFT 代码中预防伪共享？**

<details>
<summary>答案</summary>

1. 每核变量用 `__attribute__((aligned(64)))` 对齐到 cache line
2. 使用 `struct { char pad[64]; int counter; } per_cpu[N_CPUS];` 模式
3. 热数据结构避免紧凑布局，用 padding 分隔不同核访问的字段
4. 用 `perf c2c` 定期检查
5. 使用 Linux `percpu` 变量机制（内核场景）
</details>

## 参考与延伸

- [§16.2 伪共享](02-false-sharing.md) — 伪共享原理和修复
- [§16.1 MESI 协议](01-mesi.md) — 伪共享的底层机制
- [Ch15 §15.3 Cache 层次](../../chapter-15-cache-basics/notes/section-0-本章完整概述.md) — 多核 cache 架构
