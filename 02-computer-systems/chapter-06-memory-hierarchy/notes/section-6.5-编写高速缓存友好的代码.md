## 6.5 编写高速缓存友好的代码

> ↔ [Hennessy §2.3 缓存优化](../../../17-computer-architecture/chapter-02-memory-hierarchy-design/notes/section-2.3-缓存性能十项高级优化.md)


> **Ch6 §6.5** · [章导读](../README.md) · 上节 [§6.4.7 ←](./section-6.4.7-Cache参数的性能影响.md) · 下节 [§6.6 →](./section-6.6-存储器山.md)

---

**原则：**

1. **重复引用相同数据** — 时间局部性（内层循环复用）
2. **步长为 1 的顺序访问** — 空间局部性
3. **控制工作集** — fit in cache；太大则 capacity miss

**反模式：**

```c
// 差：按列扫二维数组（stride = N）
for (j = 0; j < N; j++)
    for (i = 0; i < N; i++)
        sum += a[i][j];

// 好：按行扫
for (i = 0; i < N; i++)
    for (j = 0; j < N; j++)
        sum += a[i][j];
```

**HFT 结构选择：**

| 场景 | 倾向 |
|------|------|
| 逐 tick 扫全价位 | **数组/vector** 连续 |
| 稀疏更新价位 | 哈希/树 + **节点池**（避免 scatter malloc） |
| 多字段批处理 | **SoA** |
| 单条记录热更新 | **AoS** 或紧凑 struct |

---

### 常见陷阱

1. **按列扫二维数组** — `a[i][j]` 在 C 中是行主序，按列扫（外层 j 内层 i）stride=N×8B，每次跨 cache line → 几乎每元素都 miss。改成按行扫（外层 i 内层 j）stride=8B，64B line 覆盖 8 个元素。
2. **热循环里每包 malloc 小对象** — malloc 分配的堆地址不连续，cache line 分散，每次访问可能 miss。改用预分配的连续数组/对象池。
3. **SoA 和 AoS 选错** — 批量处理某字段时 SoA（该字段连续存储）更好；单条记录多字段同时访问时 AoS 更好。选错会导致不必要的 cache miss。

### 自测题

<details>
<summary>1. 编写 cache 友好代码的三大原则是什么？</summary>

①**重复引用相同数据**——利用时间局部性（内层循环复用变量）；②**步长为 1 的顺序访问**——利用空间局部性（cache line 一次拉 64B 覆盖多个元素）；③**控制工作集大小**——让它 fit in cache，否则 capacity miss。
</details>

<details>
<summary>2. 为什么按行扫比按列扫快？具体差多少？</summary>

C 中二维数组是行主序，`a[i][j]` 和 `a[i][j+1]` 地址相邻。按行扫 stride=8B，64B cache line 覆盖 8 个 double → 每 8 元素 1 次 miss。按列扫 stride=N×8B，每元素可能跨 line → 几乎每元素都 miss。大数组时差 **10-100 倍**。
</details>

<details>
<summary>3. HFT 中 SoA 和 AoS 各适合什么场景？</summary>

**SoA**（Structure of Arrays）：每个字段单独连续存储。适合**批量处理某字段**（如遍历所有 price 做 sum）——该字段连续，cache 友好。**AoS**（Array of Structures）：每条记录的多个字段连续。适合**单条记录多字段同时访问**——一次拉 64B 覆盖整条记录。按访问模式选。
</details>

<details>
<summary>4. HFT 中为什么热循环禁止每包 malloc？</summary>

malloc 分配的堆地址**不连续**（受堆碎片影响），cache line 分散在 DRAM 各处，每次访问可能 miss。改用**预分配的连续数组/对象池**，让数据物理连续，cache line 相邻，顺序访问命中率高。
</details>

---

← [§6.4.7 ←](./section-6.4.7-Cache参数的性能影响.md) · [本章导读](../README.md) · [§6.6 →](./section-6.6-存储器山.md)
