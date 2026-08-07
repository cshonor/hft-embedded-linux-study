## 6.4.4 全相联 (S=1)

> **Ch6 §6.4.4** · [章导读](../README.md) · 上节 [§6.4.3 ←](./section-6.4.3-组相联.md) · 下节 [§6.4.5 →](./section-6.4.5-有关写的问题.md)

---

← [本章导读](../README.md)

---

### 常见陷阱

1. **以为 L1 应该用全相联** — 全相联查找需要比较所有 line 的 tag（可能上千路），延迟和功耗太高。L1 需要极低延迟，不能用全相联。全相联适合容量小、对冲突 miss 零容忍的场景。
2. **混淆 TLB 和 data cache 的相联度** — TLB 常用全相联或高路组相联（页表项少，值得全相联）；data cache L1/L2 用组相联（容量大，全相联不现实）。


### 自测题

<details>
<summary>1. 全相联（S=1）的优缺点是什么？</summary>

**优点**：消除了所有 conflict miss——任何块都可以放在任意 line，只有容量不够时才会 miss。**缺点**：查找时要比较所有 line 的 tag（可能上千路），延迟和功耗太高，不适合大容量 cache。
</details>

<details>
<summary>2. 全相联适合用在哪里？为什么？</summary>

适合**容量小、对 miss 零容忍**的场景：TLB（页表缓存，通常 64-1024 项）、小型查找表。因为项数少，全相联的并行 tag 比较延迟可控；且消除 conflict miss 对 TLB 很重要（TLB miss 代价极高）。
</details>

<details>
<summary>3. 为什么 data cache 不用全相联？</summary>

L1 data cache 32KB = 512 条 64B line，全相联需要 512 个并行 tag 比较器，延迟和面积不可接受。组相联（8-way）只需 8 个比较器，延迟低得多，且 8-way 已经消除了大部分 conflict miss。**速度优先于消除全部 conflict miss**。
</details>

---

← [§6.4.3 ←](./section-6.4.3-组相联.md) · [本章导读](../README.md) · [§6.4.5 →](./section-6.4.5-有关写的问题.md)
