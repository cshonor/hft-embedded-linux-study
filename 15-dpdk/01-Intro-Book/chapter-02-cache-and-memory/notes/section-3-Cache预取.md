## 3. Cache 预取 (Prefetching)

> 处理一个报文 = **多次内存读** — 不命中 Cache 则 **数百 cycle** 空等

---

### 一、为何要预取

典型收包路径需读：

- **RX 描述符**  
- **控制结构**（队列、ring）  
- **报文头部**（mbuf metadata + 前几个 cache line）

任一 **cache miss** 都会拖慢 **整包延迟**。

---

### 二、硬件预取

基于 **时间局部性 / 空间局部性** — CPU **自动** 预取相邻地址。

| 有效 | 无效 |
|------|------|
| **顺序** 扫描数组、描述符环 | **跳跃式**、指针 chasing、哈希随机访存 |

无效时预取 **浪费带宽**、**挤掉** 有用 cache line。

---

### 三、软件预取（DPDK 常用）

开发者 **显式** 提前加载即将用到的数据：

| 手段 | 示例 |
|------|------|
| 内联汇编 | **`PREFETCH0`** |
| Intrinsics | **`_mm_prefetch(addr, _MM_HINT_T0)`** |

**用法：** 在处理 **当前包** 时，prefetch **下一包** 的描述符 / mbuf / 数据头 — **与计算重叠** 访存延迟。

→ PMD 热路径：[chapter-03 PMD](../chapter-03-PMD与轮询模式.md)

> **深潜：** `rte_prefetch0()` 包装 — 在 rx burst 循环中常见「prefetch N+1 while handling N」。

---

← [2. Cache 层次](./section-2-阶梯式Cache系统.md) · 下一节 [4. 一致性](./section-4-Cache一致性与无锁设计.md)
