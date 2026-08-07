## 6.2 局部性（6.2.1–6.2.3）

### 6.2.1 程序数据引用的局部性

| 类型 | 定义 | 例子 |
|------|------|------|
| **时间局部性 (temporal)** | 刚访问的，很快再访问 | 循环变量 `i`、热价位节点反复更新 |
| **空间局部性 (spatial)** | 相邻地址即将访问 | 顺序扫 `price[]`、cache line 一次拉 64B |

- **步长 (stride)** 越大，空间局部性越差 — 矩阵按列扫 vs 按行扫

### 6.2.2 取指令的局部性

- 代码 **顺序执行** + 小循环重复 → I-cache 友好
- **大函数、跳转分散、ICache 压力** — 展开过度可能损 I-cache（Ch5 权衡）

### 6.2.3 局部性小结

> **局部性好的程序 ≈ 在层次结构上跑得快。**

**HFT 对照：**

| 友好 | 不友好 |
|------|--------|
| 顺序读 tick buffer | 随机跳链表 deep chase |
| 固定大小 ring array | 每包 `new` 小对象 |
| 热 struct 字段紧凑 |  giant object 跨多条 cache line |

→ 优化循环：[Ch 5](../chapter-05-optimizing-performance/)

---

### 常见陷阱

1. **只关注数据局部性忽略指令局部性** — 大函数/跳转分散的代码 I-cache miss 高。过度展开（§5.8）可能导致代码膨胀超出 I-cache 容量，反而变慢。
2. **链表 vs 数组不区分场景** — 链表节点分散在堆上，cache line 不连续，每次跳转可能 miss。HFT 热路径用连续数组/vector 替代链表，或用节点池预分配保证连续。
3. **以为小对象就一定 cache 友好** — 如果小对象分散 malloc（堆碎片），物理地址不连续，仍然 cache 不友好。关键是**逻辑连续 + 物理连续**。

### 自测题

<details>
<summary>1. 时间局部性和空间局部性分别是什么？各举一个 HFT 例子。</summary>

- **时间局部性**：刚访问的数据很快再访问。例：热价位节点在多个 tick 中反复更新。
- **空间局部性**：相邻地址即将被访问。例：顺序扫描 `price[]` 数组，cache line 一次拉 64B 覆盖 8 个 double。
</details>

<details>
<summary>2. 步长（stride）如何影响空间局部性？</summary>

步长越大，每次访问跳过的字节越多，空间局部性越差。按行扫二维数组（stride=1）每 64B 拉 1 次 cache line，效率高；按列扫（stride=N×8B）每次访问可能落在不同 cache line，每元素都 miss。
</details>

<details>
<summary>3. HFT 中链表为什么 cache 不友好？怎么改？</summary>

链表节点分散在堆上（malloc 分配），物理地址不连续。遍历时每次 `next` 跳转可能落到不同 cache line → **cache miss**。改用连续数组/vector，或用**节点池**预分配一块连续内存，让节点物理相邻。
</details>

---

← [本章导读](../README.md)
