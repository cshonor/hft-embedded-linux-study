# §20.1 独占监视器（Exclusive Monitor）

> **来源：** [Ch20 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

ARM 用 LDXR + STXR 配对实现原子操作：LDXR 标记独占访问，STXR 检查是否被干扰。独占监视器是缓存行级别的——其他核写了该行则监视器被清除，STXR 失败。本节分析 LDXR/STXR 工作原理、监视器粒度和伪共享对原子操作的影响。

## 核心要点

### LDXR / STXR 指令

| 指令 | 行为 | 说明 |
|------|------|------|
| `LDXR Wd/Xd, [Xn]` | 独占加载：标记"我正在独占访问这个地址" | 设置独占监视器 |
| `STXR Ws, Wd/Xd, [Xn]` | 独占存储：如果监视器仍有效→写入成功(Ws=0)；否则失败(Ws=1) | 返回成功/失败 |
| `CLREX` | 清除本地独占监视器（放弃独占） | CAS 不匹配时用 |

### 工作原理

```
CPU0                          CPU1
ldxr x0, [addr]               
                              ldxr x1, [addr]  ← 也标记独占
                              stxr w2, x1, [addr]  ← 成功(w2=0)，CPU0的监视器被清除
stxr w2, x0, [addr]           
← 失败(w2=1)，必须重试
```

> 独占监视器是**缓存行级别**的：监视一个 Cache Line，如果其他核写了该行→监视器被清除。

### 独占监视器状态

```
LDXR → 标记独占（监视器 Active）
  │
  ├── 其他核写了该 cache line → 监视器被清除（Passive）
  │     └── STXR → 失败(Ws=1) → 重试
  │
  ├── 本核写了该地址 → 正常完成
  │     └── STXR → 成功(Ws=0)
  │
  ├── 异常/上下文切换 → 自动 CLREX
  │     └── STXR → 失败(Ws=1) → 重试
  │
  └── 超时 → 监视器自动过期（实现相关）
        └── STXR → 可能失败
```

### STXR 返回值

| Ws 值 | 含义 | 行动 |
|-------|------|------|
| 0 | 写入成功 | 继续（原子操作完成） |
| 1 | 写入失败（被干扰） | 重试（循环回 LDXR） |

### 独占监视器类型

| 类型 | 粒度 | 说明 |
|------|------|------|
| Local Monitor | 本核 | 监视本核的 LDXR/STXR 配对 |
| Global Monitor | 全局 | 多核场景，监视跨核的独占访问 |
| 监视范围 | Cache Line | 通常 64 字节，一个 cache line 内的任何写都清除监视器 |

### 伪共享对原子操作的影响

```c
// 两个原子变量在同一 cache line
struct {
    std::atomic<int> counter_a;  // offset 0, CPU0 操作
    std::atomic<int> counter_b;  // offset 4, CPU1 操作
} counters;  // 两个原子变量在同一 cache line

// CPU0 对 counter_a 做 atomic_add
// → LDXR [counter_a] → 标记整个 cache line 独占
// CPU1 对 counter_b 做 atomic_add
// → LDXR [counter_b] → 清除 CPU0 的监视器
// CPU0 的 STXR → 失败！即使操作的是不同变量

// 修复：让原子变量独占 cache line
struct alignas(64) {
    std::atomic<int> counter_a;  // 独占 cache line
} cpu0_counter;
struct alignas(64) {
    std::atomic<int> counter_b;  // 独占 cache line
} cpu1_counter;
```

## HFT 关联

LDXR/STXR 是 ARM 原子操作的基础，HFT 无锁数据结构（如 MPSC 队列、订单簿并发更新）依赖它。

### HFT 原子变量布局建议

```c
// ✓ 每核独立原子计数器（避免伪共享）
struct alignas(64) per_cpu_counters {
    std::atomic<uint64_t> order_count;
    std::atomic<uint64_t> cancel_count;
    std::atomic<uint64_t> latency_sum;
    char pad[40];  // 填充到 64 字节
} counters[MAX_CPUS];

// ✗ 多核共享原子变量（伪共享）
struct {
    std::atomic<uint64_t> order_count;  // 多核竞争同一 cache line
} shared_counter;
```

独占监视器的缓存行粒度意味着：如果两个不相关的变量在同一 cache line 中，一个核的 LDXR/STXR 操作可能导致另一个核的 STXR 失败——这是伪共享在原子操作中的表现。HFT 中应确保原子变量独占 cache line（`aligned(64)`）。在高竞争场景下，LDXR/STXR 循环可能反复失败（livelock），ARMv8.1 LSE 单指令原子操作可避免此问题。

## 自测题

1. **LDXR/STXR 如何实现原子操作？STXR 返回 1 代表什么？**

<details>
<summary>答案</summary>

LDXR 标记独占监视器（"我正在独占访问这个地址"），STXR 检查监视器是否仍有效：有效→写入成功（返回 0）；无效→写入失败（返回 1）。如果两个核同时 LDXR 同一地址，先 STXR 的核成功并清除另一核的监视器，后 STXR 的核失败（返回 1），必须重试。返回 **1 = 失败**（需重试）。
</details>

2. **独占监视器的粒度是什么？如果两个变量在同一 cache line 会怎样？**

<details>
<summary>答案</summary>

独占监视器是**缓存行级别**（通常 64 字节）的。如果两个不相关的变量在同一 cache line 中，一个核对变量 A 做 LDXR/STXR，另一个核对变量 B 做 LDXR/STXR 会导致前者的监视器被清除（STXR 失败）——即使两个核操作的是不同变量。这就是**伪共享对原子操作的影响**。修复：用 `aligned(64)` 让原子变量独占 cache line。
</details>

3. **CLREX 指令的作用是什么？什么时候需要用？**

<details>
<summary>答案</summary>

CLREX 清除本地独占监视器（放弃独占）。当 LDXR 后决定不执行 STXR（如 CAS 比较发现值不匹配，不需要写入）时，应执行 CLREX 清除监视器。不执行 CLREX 的话监视器会残留，可能影响后续 LDXR 的行为（架构实现相关）。在异常处理中也会自动 CLREX（防止异常前残留的监视器影响异常处理）。
</details>

4. **HFT 中两个核分别对两个不相关原子变量做 atomic_add，但 STXR 反复失败，可能是什么原因？**

<details>
<summary>答案</summary>

**伪共享**。两个原子变量在同一 cache line 中。CPU0 对变量 A 做 LDXR 标记整个 cache line 独占，CPU1 对变量 B 做 LDXR 清除了 CPU0 的监视器，CPU0 的 STXR 失败——即使操作的是不同变量。修复：用 `__attribute__((aligned(64)))` 或 `alignas(64)` 让每个原子变量独占一个 cache line。
</details>

## 参考与延伸

- [§20.2 原子操作实现模式](02-atomic-patterns.md) — LDXR/STXR 循环代码
- [§20.3 ARMv8.1 LSE](03-lse.md) — 替代 LDXR/STXR 的单指令方案
- [Ch19 §19.6 HFT 中的屏障使用](../../chapter-19-barrier-usage/notes/section-0-本章完整概述.md) — 无锁队列中的应用
