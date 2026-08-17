# Item 1：仔细选择容器

> 第 1 章 容器 · Item 1 · 下一节：[Item 2 不要写容器无关代码](item02-no-container-agnostic.md)

## 为什么要学这个（先建立直觉）

C 程序员只有一个"容器"——数组：

```c
int orders[1000];  // 固定大小，无法动态增长
// 查找：线性扫描或自排序 + bsearch
// 插入：memmove 后移，O(n)
// 删除：memmove 前移，O(n)
```

C++ STL 提供了十几种容器，各有取舍。选错容器，算法复杂度可能从 O(1) 变 O(n)，cache 命中率从 90% 掉到 10%。

```cpp
std::vector<int> v;        // 连续内存，cache 友好，尾插 O(1) 均摊
std::list<int> l;          // 链表，中间插删 O(1)，但 cache 不友好
std::map<int, Data> m;     // 红黑树，查找 O(log n)，有序
std::unordered_map<int, Data> um;  // 哈希表，查找 O(1) 均摊
```

选容器不是选"最好的"，而是选"最适合场景的"。

---

## 这节讲什么

STL 容器分为两大阵营——**连续内存容器**（vector/deque/string/array）和**节点容器**（list/forward_list/map/set/unordered_*）。两大阵营在 cache 局部性、插入删除复杂度、迭代器失效规则上截然不同。本章帮你建立"需求→容器"的映射。

---

## 容器选型决策表

| 需求 | 首选容器 | 理由 |
|------|----------|------|
| 连续内存、随机访问、尾插 | `vector` | cache 友好，尾插均摊 O(1) |
| 头尾双端操作 | `deque` | 两端 O(1)，中间 O(n) |
| 频繁中间插删 | `list` / `forward_list` | 节点容器，插删 O(1) |
| 有序键值查找 | `map` / `set` | 红黑树 O(log n) |
| 无序快速查找 | `unordered_map` / `unordered_set` | 哈希 O(1) 均摊 |
| 小整数键密集 | `vector` 直接下标 | 比 map 快一个数量级 |
| 固定大小数组 | `array` | 栈分配，零开销 |

### 连续 vs 节点：核心权衡

```cpp
// 连续内存：cache 友好但中间插删 O(n)
std::vector<int> v = {1, 2, 3, 4, 5};
v.insert(v.begin() + 2, 99);  // 3,4,5 全部后移 → O(n)

// 节点容器：中间插删 O(1) 但 cache 不友好（指针追逐）
std::list<int> l = {1, 2, 3, 4, 5};
auto it = std::next(l.begin(), 2);
l.insert(it, 99);  // 只改指针 → O(1)
// 但遍历时每次 ++it 都跳到不同内存地址 → cache miss
```

---

## 常见错误（新手踩坑）

### 错误 1：为"频繁插入"选 list，忽略 cache 代价

```cpp
// 以为 list 插入快就选 list
std::list<int> l;
for (int i = 0; i < 100000; ++i) l.push_back(i);
// 遍历比 vector 慢 5-10 倍——每节点独立分配，cache miss 严重
```

**修正：** 除非中间插删频率远超遍历，否则选 `vector`。现代 CPU cache 局部性比算法复杂度更决定实际性能。

### 错误 2：用 map 存小整数键

```cpp
std::map<int, Exchange> exchanges;
exchanges[0] = Exchange("NYSE");
exchanges[1] = Exchange("NASDAQ");
// 只有 8 个交易所（ID 0-7），用 map 是杀鸡用牛刀
```

**修正：** `std::vector<Exchange> exchanges(8);` 直接下标访问，O(1) 且 cache 友好。

### 错误 3：用 deque 模拟栈但忘了头部的存在

```cpp
std::deque<int> d;
d.push_back(1);
d.push_back(2);
d.pop_front();  // 以为只删了"旧的"，但 deque 是双端的
// 如果只想栈语义，用 std::stack 适配器更安全
```

**修正：** 栈用 `std::stack<T>`（默认适配 `deque`），队列用 `std::queue<T>`。

---

## 新手要点（和 C 的区别）

| 维度 | C | C++ STL | 为什么 |
|------|---|---------|--------|
| 容器类型 | 只有数组 | 十几种容器 | 场景匹配 |
| 动态增长 | `realloc`（可能搬移） | `vector` 自动扩容 | 异常安全 |
| 查找 | `bsearch` / 线性扫描 | `find` / `map::find` / `unordered_map::find` | 复杂度可选 |
| 内存布局 | 数组连续 | 看容器：连续 or 节点 | cache 影响 |
| 删除 | `memmove` | `erase` / `remove`+`erase` | 语义清晰 |

**一句话：** C 只有数组，所有数据结构手写。STL 提供了选型矩阵——先看需求（查找/插删/遍历），再选容器，最后考虑 cache。

---

## HFT 关联

- **`vector` 连续存储换 cache**：订单簿档位、tick 缓冲用 `vector`，顺序遍历 cache 命中率高；`map`/`list` 的指针追逐在每 tick 路径上引入 cache miss，延迟尖峰。
- **`reserve` 预留**：知道峰值容量的 `vector`/`unordered_map` 预 `reserve`，避免热路径扩容 + rehash。
- **小整数键用 `vector`**：交易所 ID（0-7）、订单类型（0-3）用 `vector` 直接下标，比 `map` 快一个数量级且零哈希开销。

---

## 代码自测

### Q1: 容器选型
```cpp
// 场景：存储 10 万个订单 ID（int），需要：
// 1. 快速查找某 ID 是否存在
// 2. 按顺序遍历
// 3. 偶尔删除

// A
std::vector<int> ids;
// B
std::set<int> ids;
// C
std::unordered_set<int> ids;
```
> 三个方案各有什么优劣？HFT 场景选哪个？

<details>
<summary>答案</summary>

| 方案 | 查找 | 顺序遍历 | 删除 | cache |
|------|------|---------|------|-------|
| `vector`+sort | O(log n) 二分 | ✅ 连续 | O(n) | ✅ 友好 |
| `set` | O(log n) | ✅ 有序 | O(log n) | ❌ 指针追逐 |
| `unordered_set` | O(1) 均摊 | ❌ 无序 | O(1) | ❌ 不友好 |

**HFT 选 vector + sort + binary_search**：10 万 int 排序后 binary_search O(log n)，连续存储 cache 友好。删除少可以标记删除（tombstone）+ 定期压缩。
</details>

### Q2: list vs vector 遍历性能
```cpp
std::vector<int> v(100000);
std::list<int> l(100000);
// 都填充 0-99999
// 遍历求和
```
> 哪个遍历更快？为什么？

<details>
<summary>答案</summary>

**vector 快 5-10 倍**。vector 元素连续存储，CPU 预取器能预测访问模式，cache line（64B）一次载入 16 个 int。list 每节点独立分配，`++it` 跳到随机地址，每次都是 cache miss。

即使 list 插入 O(1) 比 vector O(n) 快，但只要遍历频率 > 插入频率，vector 总赢。
</details>

### Q3: deque 的双端特性
```cpp
std::deque<int> d = {1, 2, 3};
d.push_front(0);
d.push_back(4);
// d 的内容？
```
> deque 和 vector 在内存布局上有什么本质区别？

<details>
<summary>答案</summary>

d = {0, 1, 2, 3, 4}。

**deque 内存布局**：deque 不是一块连续内存，而是多个固定大小块（chunk）的数组。头尾各预留空间，所以 `push_front`/`push_back` 都是 O(1)。但随机访问需要两次解引用（先找块再找偏移），比 vector 略慢。

**vector**：一块连续内存，只能尾插。头部插入 O(n)。
</details>

### Q4: array vs vector
```cpp
std::array<int, 5> a = {1, 2, 3, 4, 5};
std::vector<int> v = {1, 2, 3, 4, 5};
// a 和 v 的 sizeof 分别大约多少？哪个在栈上？
```

<details>
<summary>答案</summary>

- `sizeof(a)` = 20（5 × 4 字节，纯数据，无额外开销）
- `sizeof(v)` ≈ 24（3 个指针：begin/end/capacity，数据在堆上）

`array` 在栈上（如果是局部变量），`vector` 的数据在堆上。`array` 是 C 数组的零开销包装，适合固定大小场景。
</details>

---

## 参考与延伸

- 下一节：[Item 2 不要写容器无关代码](item02-no-container-agnostic.md)
- 回到：[第 1 章 容器](README.md)
