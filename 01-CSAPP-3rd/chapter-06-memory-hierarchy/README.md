# Ch 6 存储器层次结构 · The Memory Hierarchy

> **CSAPP 3rd** · Bryant & O'Neill · **精读 🔴**（Part I）

> 本章定位：**为什么 cache miss 比算慢** — 从 DRAM/SSD 到 L1/L3，靠 **局部性** 让层次结构有效；学会写 **cache-friendly** 代码与读 **存储器山**。HFT 地基篇核心，与 [02-Hennessy Ch2](../../02-Computer-Architecture-6th/) 交叉。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 6.1 存储技术（6.1.1–6.1.4） | [notes/section-6.1-存储技术.md](./notes/section-6.1-存储技术.md) |
| 6.2 局部性（6.2.1–6.2.3） | [notes/section-6.2-局部性.md](./notes/section-6.2-局部性.md) |
| 6.3 存储器层次结构（6.3.1–6.3.2） | [notes/section-6.3-层次结构与缓存概念.md](./notes/section-6.3-层次结构与缓存概念.md) |
| 6.4.1–6.4.4 Cache 组织与映射 | [notes/section-6.4.1-6.4.4-Cache组织与映射.md](./notes/section-6.4.1-6.4.4-Cache组织与映射.md) |
| 6.4.5–6.4.7 写策略与真实 Cache | [notes/section-6.4.5-6.4.7-写策略与真实Cache层次.md](./notes/section-6.4.5-6.4.7-写策略与真实Cache层次.md) |
| 6.5–6.6 缓存友好代码与存储器山 | [notes/section-6.5-6.6-缓存友好代码与存储器山.md](./notes/section-6.5-6.6-缓存友好代码与存储器山.md) |

---

## 大白话 · 本章一条线

> **内存不是「一个速度」—— 离 CPU 越近越小越快；程序快不快，看能不能命中近处。**

```
寄存器 → L1 → L2 → L3 → DRAM → SSD/HDD
         ↑ 靠 时间/空间 局部性 自动缓存
```

**HFT 三件事：**

1. **热数据 fit 在 cache** — order book、ring buffer 顺序访问
2. **避免 false sharing** — 多线程别写同一 cache line（→ 3.9、[Ch 12](../chapter-12-concurrent-programming/)）
3. **冷路径才碰磁盘/远程内存** — tick 路径零 I/O

---

## 本章 Checklist

- [ ] 区分 SRAM(D cache) vs DRAM vs 磁盘/SSD 数量级延迟
- [ ] 解释 **时间局部性**、**空间局部性** 各举 HFT 例子
- [ ] 会算：地址 → `{tag, set index, block offset}`（给定 s,E,B）
- [ ] 区分 **直接映射 / 组相联 / 全相联**；冲突 miss vs 容量 miss vs 冷 miss
- [ ] 知道 **写直达 vs 写回**、**write allocate**
- [ ] 能改 **循环顺序** 提高空间局部性（存储器山实验）
- [ ] 会用 `perf stat` 看 `cache-misses`、`LLC-load-misses`

---

## HFT 精读捷径

```
6.2 局部性 + 6.5 友好代码 — 日常编码
6.4 Cache 映射 — 理解 false sharing、对齐、padding
6.6 存储器山 — 一次实验建立直觉
6.1 存储技术 — 扫读；SSD/NVMe 细节在 SysPerf Ch9
配套：02-Hennessy Ch2 · 03-SysPerf Ch6/Ch7 · DPDK mbuf 池
```

---

## 相关章节

- 上一章：[../chapter-05-optimizing-performance/](../chapter-05-optimizing-performance/)
- 下一章：[../chapter-07-linking/](../chapter-07-linking/)
- 虚拟内存：[../chapter-09-virtual-memory/](../chapter-09-virtual-memory/)
- 理论：[02-Hennessy Ch2](../../02-Computer-Architecture-6th/)
- 观测：[03-SysPerf Ch 6 CPUs](../../03-Systems-Performance-2nd/chapter-06-cpus/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
