# §16.2 伪共享（False Sharing）

> **来源：** [Ch16 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

不同核频繁写同一 cache line 的不同变量时，MESI 协议导致该行在核间反复搬运（invalidate → reload），性能暴跌。修复方法：对齐填充让不同核的变量不在同一 cache line。

## 核心要点

### 伪共享机制

```c
// 两个变量在同一 Cache Line（64字节）内
struct {
    int a;    // offset 0，CPU0 频繁写
    int b;    // offset 4，CPU1 频繁写
    // padding... 共 64 字节
} data;
```

- CPU0 写 `a` → Cache 行变 M → CPU1 的同一行变 I
- CPU1 写 `b` → 要重新加载 → 变 M → CPU0 的行变 I
- 循环往复 → Cache 行在两核间反复搬运 → 性能暴跌

### 修复方法

```c
// 方法1：手动 padding
struct {
    int a;
    char pad[60];   // 填充到 64 字节
    int b;
} data;

// 方法2：GCC 属性对齐
struct data {
    int a __attribute__((aligned(64)));
    int b __attribute__((aligned(64)));
};

// 方法3：Linux 内核宏
____cacheline_aligned
```

### 伪共享 vs 真共享

| 类型 | 原因 | 是否有意的 |
|------|------|-----------|
| 真共享 | 不同核访问同一变量（需要同步） | 是（设计如此） |
| 伪共享 | 不同核访问不同变量，但在同一 cache line | 否（应避免） |

## HFT 关联

伪共享是 HFT 多核系统中最常见的性能陷阱。HFT 系统通常每核维护独立的统计计数器（如每核的订单计数），如果这些计数器在同一 cache line 中，每次更新都会触发 MESI 广播。修复方法：用 `__attribute__((aligned(64)))` 让每核变量独占一个 cache line。Linux 内核的 `percpu` 变量就是通过类似机制避免伪共享。用 `perf c2c` 工具可以检测伪共享。

## 自测题

1. **什么是伪共享？它为什么会导致性能下降？**

<details>
<summary>答案</summary>

伪共享：不同 CPU 核频繁写**同一 cache line 的不同变量**。因为 MESI 协议以 cache line 为单位管理一致性，写操作会让其他核的该行 invalidate。结果：cache line 在核间反复搬运（每次写都要重新加载），性能暴跌。虽然不同核写的是不同变量，但共享了 cache line → "伪"共享。
</details>

2. **如何修复伪共享？写出两种方法。**

<details>
<summary>答案</summary>

方法1：**手动 padding** 填充到 cache line 大小（64 字节）
```c
struct { int a; char pad[60]; int b; } data;
```

方法2：**GCC 属性对齐**
```c
struct data {
    int a __attribute__((aligned(64)));
    int b __attribute__((aligned(64)));
};
```

两种方法都让 a 和 b 在不同 cache line 中，消除伪共享。
</details>

3. **如何用 perf 工具检测伪共享？**

<details>
<summary>答案</summary>

用 `perf c2c`（Cache-to-Cache）工具检测：
```bash
perf c2c record ./program
perf c2c report
```
报告中的 "Hitm"（Hit in Modified）指标高表示跨核 cache line 竞争严重，可能存在伪共享。也可以用 `perf stat -e cache-misses` 观察 cache miss 异常高的区域。
</details>

## 参考与延伸

- [§16.1 MESI 协议](01-mesi.md) — 伪共享的底层原因
- [§16.5 实验要点](05-lab.md) — 实验 16-1 伪共享性能对比
- [Ch15 §15.4 关键概念](../../chapter-15-cache-basics/notes/section-0-本章完整概述.md) — Cache line 大小
