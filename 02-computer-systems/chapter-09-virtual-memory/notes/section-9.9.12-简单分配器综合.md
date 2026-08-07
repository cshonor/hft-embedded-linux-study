## 9.9.12 简单分配器综合

> **Ch9 §9.9.12** · [章导读](../README.md) · 上节 [§9.9.10-9.9.11 ←](./section-9.9.10-9.9.11-合并与边界标记.md) · 下节 [§9.9.13 →](./section-9.9.13-显式空闲链表.md)

---

- 课程 lab — 理解 `mm_malloc` / `mm_free` / `mm_realloc`

---

### CSAPP Malloc Lab 概述

- **目标：** 实现 `mm_init` / `mm_malloc` / `mm_free` / `mm_realloc`
- **约束：** 只能调 `mem_sbrk` 扩展堆，不能直接用系统 malloc
- **评分：** 空间利用率 × 吞吐量

| 函数 | 职责 |
|------|------|
| `mm_init` | 初始化堆：prologue + epilogue 块 |
| `mm_malloc` | find_fit → split → 返回 payload 指针 |
| `mm_free` | 标记空闲 → coalesce 前后 |
| `mm_realloc` | 特殊情况优化（原地扩展/缩小），否则 malloc+copy+free |

**优化路径：** 隐式链表 → 显式链表 → 分离链表 → 分离适配（最佳）

### 常见陷阱
1. **Lab 的 mm_malloc 不等于 glibc malloc** — Lab 是教学简化版，glibc 用更复杂的 ptmalloc/tcmalloc/jemalloc
2. **realloc 不一定原地扩展** — 可能 malloc 新块 + memcpy + free 旧块，数据量大时开销显著
3. **测试用 trace 文件验证利用率和吞吐** — 利用率 = peak_payload / peak_heap，不只是不泄漏

### 自测题

<details>
<summary>Q1: CSAPP Malloc Lab 需要实现哪些函数？评分标准是什么？</summary>

实现 mm_init/mm_malloc/mm_free/mm_realloc。评分 = 空间利用率（peak_payload / peak_heap）× 吞吐量（ops/sec）。

</details>

<details>
<summary>Q2: mm_init 需要做什么？prologue 和 epilogue 块的作用？</summary>

mm_init 调 mem_sbrk 创建初始堆结构。prologue 块（已分配，永久存在）防止合并越界到堆前；epilogue 块（size=0, alloc=1）标记堆尾，扩展堆时移动 epilogue。

</details>

<details>
<summary>Q3: realloc 的优化策略有哪些？</summary>

1) 缩小：原地改 header，分割尾部；2) 下一块空闲且够大：合并下一块，原地扩展；3) 否则：malloc 新块 + memcpy + free 旧块。

</details>

<details>
<summary>Q4: 从隐式链表到分离适配，性能提升的路径是什么？</summary>

隐式链表 O(n) → 显式链表 O(空闲块数) → 分离链表（按大小分桶）O(桶内空闲块数) → 分离适配（桶内首次适配）≈O(1)。

</details>

---

← [§9.9.10-9.9.11 ←](./section-9.9.10-9.9.11-合并与边界标记.md) · [本章导读](../README.md) · [§9.9.13 →](./section-9.9.13-显式空闲链表.md)
