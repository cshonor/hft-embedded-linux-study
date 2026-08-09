## 6.3 存储器层次结构（6.3.1–6.3.2）

> ↔ [Hennessy §2.1 存储器层次](../../../19-computer-architecture/chapter-02-memory-hierarchy-design/notes/section-2.1-引言与存储器层次.md)


### 6.3.1 层次结构中的缓存

**核心思想：** 第 k+1 层是第 k 层的 **cache**，由硬件或软件管理。

```
L0 寄存器
L1 d-cache / i-cache（单核私有 · 指令/数据分离）
L2 统一 cache（单核私有）
L3 = LLC（Last Level Cache · 常多核共享）
主存 DRAM
本地磁盘 / 远程存储
```

**LLC 口述：** 片上 **最后一级**；主流三级系统里 **L3 = LLC**。详解 → [Ch1 §1.5](../../chapter-01-tour-of-computer-systems/notes/section-1.5-高速缓存至关重要.md)。

- **命中 (hit)** — 在上层找到
- **缺失 (miss)** — 向下层取，**惩罚 latency**
- **块 (block/line)** — 以块为单位搬移，利用空间局部性

### 6.3.2 概念小结

| 术语 | 含义 |
|------|------|
| **块大小 B** | 通常 64B |
| **相联度 E** | 每组几条 line |
| **组数 S** | 索引组数 |
| **容量** | ≈ S × E × B（简化） |

**AMAT (平均访问时间)：**

```
AMAT = HitTime + MissRate × MissPenalty
```

**HFT：** 优化目标常是 **降 miss rate** 或 **降 miss penalty**（如 NUMA 本地内存、prefetch）；`perf` 量化 miss；关注 **LLC miss**（共享层与跨核）。

→ [Ch 1.5 缓存直觉 · LLC](../../chapter-01-tour-of-computer-systems/notes/section-1.5-高速缓存至关重要.md)

---

### 常见陷阱

1. **混淆 hit rate 和 hit time** — hit rate 高不代表快；如果 hit time 本身高（如 L3 比 L1 慢 10 倍），高 hit rate 仍然慢。AMAT = HitTime + MissRate × MissPenalty，三者都要看。
2. **以为 cache 容量越大越好** — 大 cache 容量意味着更多组/路，查找延迟更高（L3 比 L1 慢就是因为大且远）。层次结构是速度-容量的折中，不是简单「越大越好」。
3. **忽略 cache line 大小** — 即使只读 1 字节，CPU 也会拉整条 cache line（64B）。如果访问模式跨 line（如 misaligned），一次访问触发多次 cache 行填充。

### 自测题

<details>
<summary>1. 存储器层次结构的核心思想是什么？</summary>

第 k+1 层是第 k 层的 **cache**。上层快但小（寄存器→L1→L2→L3），下层慢但大（DRAM→磁盘）。数据按**块（cache line）**搬移，利用空间局部性。目标是让常用数据留在上层，不常用的留在下层。
</details>

<details>
<summary>2. AMAT 公式是什么？各部分含义？</summary>

**AMAT = HitTime + MissRate × MissPenalty**。HitTime = 命中时的访问时间；MissRate = 缺失概率；MissPenalty = 缺失时向下层取数据的惩罚。HFT 优化方向：降 miss rate（cache 友好布局）或降 miss penalty（预取、NUMA 本地内存）。
</details>

<details>
<summary>3. 为什么 L1 比 L3 快？不只是「更近」。</summary>

L1 容量小（32KB）→ 组数/路数少 → 查找延迟低（tag 比较快）；L3 容量大（数十 MB）→ 查找延迟高。此外 L1 在核心内部（物理距离近），L3 常多核共享（总线更长）。**容量和延迟是折中**——大 cache 查找慢。
</details>

---

← [本章导读](../README.md)
