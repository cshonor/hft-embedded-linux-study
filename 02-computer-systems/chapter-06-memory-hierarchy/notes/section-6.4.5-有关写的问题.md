## 6.4.5 有关写的问题

> **Ch6 §6.4.5** · [章导读](../README.md) · 上节 [§6.4.4 ←](./section-6.4.4-全相联.md) · 下节 [§6.4.6 →](./section-6.4.6-真实Cache层次解剖.md)

---

| 策略 | 行为 |
|------|------|
| **写直达 (write-through)** | 写同时更新 cache 与下层 — 简单，总线忙 |
| **写回 (write-back)** | 只写 cache，**dirty** 位；替换时才写回 — 常用 |
| **写分配 (write-allocate)** | miss 时先 load line 再写 — 利用局部性 |
| **非写分配** | miss 直接写下层 — 少用于 L1 |

- **store 引发 miss** — 可能触发 load line（与 Ch5 load 性能联动）

---

### 常见陷阱

1. **混淆 write-through 和 write-back** — write-through 每次写都同步到下层（简单但总线忙）；write-back 只写 cache，标记 dirty 位，替换时才写回（常用但一致性复杂）。现代 L1/L2 用 write-back。
2. **store miss 时不知道会发生什么** — write-allocate 策略下，store miss 会先 load 整条 cache line 再写入（利用空间局部性）；non-write-allocate 直接写下层。L1 常用 write-allocate。
3. **忽略 store 引发的隐式 load** — write-allocate 下，即使只写 1 字节，也要先从下层拉 64B 的 cache line。如果下层 miss，这个隐式 load 的 miss penalty 和普通 load miss 一样高。

### 自测题

<details>
<summary>1. write-through 和 write-back 的区别？现代 cache 用哪个？</summary>

**Write-through**：写 cache 同时写下层——简单、一致性好，但每次写都占用总线，功耗高。**Write-back**：只写 cache，标记 dirty 位，替换时才写回——总线占用少、功耗低，但一致性管理复杂。现代 L1/L2 **常用 write-back**。
</details>

<details>
<summary>2. write-allocate 和 non-write-allocate 的区别？store miss 时各发生什么？</summary>

**Write-allocate**：store miss 时先从下层 load 整条 cache line，再写入——利用空间局部性（附近数据可能很快被读）。**Non-write-allocate**：store miss 时直接写到下层，不拉 cache line。L1 常用 write-allocate + write-back。
</details>

<details>
<summary>3. 为什么 store miss 也可能产生 cache miss 延迟？</summary>

Write-allocate 策略下，store miss 会触发**隐式 load**——从下层拉 64B cache line。如果下层也 miss（如 LLC miss 到 DRAM），这个 load 的 miss penalty 和普通 load miss 一样高（~100ns）。HFT 热路径应避免对不在 cache 中的地址做 store。
</details>

---

← [§6.4.4 ←](./section-6.4.4-全相联.md) · [本章导读](../README.md) · [§6.4.6 →](./section-6.4.6-真实Cache层次解剖.md)
