# Item 14：用 reserve 避免不必要的重新分配

> 第 2 章 vector 和 string · Item 14 · 上一节：[Item 12-13 容量与扩容](item12-13-capacity-reserve.md) · 下一节：[Item 15 string 实现多样性](item15-string-implementation.md)

## 为什么要学这个（先建立直觉）

C 程序员知道提前分配足够内存的重要性：

```c
// 不好的做法：频繁 realloc
char* buf = malloc(1);
for (int i = 0; i < 1000; ++i) {
    buf = realloc(buf, i + 2);  // 可能每次都搬移！
    buf[i] = data[i];
}

// 好的做法：一次分配够
char* buf = malloc(1000);
for (int i = 0; i < 1000; ++i) buf[i] = data[i];
```

C++ 的 `vector` 同理——`reserve` 是 `vector` 性能调优的第一手段。

```cpp
std::vector<Tick> ticks;
ticks.reserve(estimate);          // 一次性分配
for (...) ticks.push_back(tick);  // 零扩容
```

---

## 这节讲什么

`reserve(n)` 预分配容量，让后续 `push_back` 不触发扩容。对 `unordered_map` 同理——`reserve(bucket_count)` 避免 rehash 尖峰。`reserve` 是消除热路径延迟尖峰的关键手段。

---

## reserve 的正确用法

```cpp
// vector
std::vector<int> v;
v.reserve(10000);  // 预分配 10000 个 int 的空间
for (int i = 0; i < 10000; ++i)
    v.push_back(i);  // 零扩容，零迭代器失效

// unordered_map
std::unordered_map<int, std::string> m;
m.reserve(10000);  // 预分配桶，避免 rehash
for (int i = 0; i < 10000; ++i)
    m[i] = "value";  // 零 rehash

// string
std::string s;
s.reserve(1024);  // 预分配
for (char c : data) s += c;  // 零扩容
```

### 什么时候用 reserve

```cpp
// ✅ 知道（或能估计）最终大小
std::vector<Order> orders;
orders.reserve(max_orders_per_tick);  // HFT 启动时按峰值预分配

// ✅ 批量插入前
v.reserve(v.size() + src.size());
v.insert(v.end(), src.begin(), src.end());

// ❌ 不知道大小，只是盲目 reserve
v.reserve(1000000);  // 如果最终只用了 10 个 → 浪费 4MB
```

---

## 常见错误（新手踩坑）

### 错误 1：reserve 后忘了 size 还是 0

```cpp
std::vector<int> v;
v.reserve(100);
std::cout << v.size();   // 0！reserve 不改变 size
// v[0] = 42;  // UB！size 是 0，越界
```

**修正：** `reserve` 只改变 `capacity`，不改变 `size`。要用 `resize(n)` 来同时改变 size（会值初始化新元素）。

### 错误 2：reserve 过大浪费内存

```cpp
std::vector<int> v;
v.reserve(10000000);  // 40MB，但实际只用了 100 个
// 浪费 39.6MB 内存
```

**修正：** 按实际峰值 reserve，不要盲目预留过大空间。

### 错误 3：对 list/forward_list 调用 reserve

```cpp
std::list<int> l;
// l.reserve(100);  // 编译错误！list 没有 reserve
```

**修正：** `reserve` 是 `vector`/`string`/`unordered_*` 的接口。`list` 不需要预分配（每个节点独立分配）。

---

## 新手要点（和 C 的区别）

| 维度 | C | C++ STL | 为什么 |
|------|---|---------|--------|
| 预分配 | `malloc(n)` 直接 | `reserve(n)` | 分离分配与构造 |
| 扩容 | `realloc` | 自动翻倍 | 均摊 O(1) |
| 大小 vs 容量 | `n` vs `capacity` | `size()` vs `capacity()` | 明确区分 |

**一句话：** C 的 `malloc(n)` 一步到位分配+可用。C++ 的 `reserve(n)` 只分配不构造——`size` 不变，`capacity` 变。这是"分配与构造分离"的设计。

---

## HFT 关联

- **启动时 reserve 峰值容量**：HFT 系统启动时按峰值行情量 `reserve`，热路径零扩容、零迭代器失效。
- **`unordered_map::reserve` 避免 rehash**：订单 ID→订单对象映射，预 `reserve` 避免 rehash 导致的全表重哈希尖峰。
- **`reserve` + `emplace_back` = 零分配热路径**：预 reserve + 原地构造 = 热路径上零 `malloc`、零拷贝、零迭代器失效。

---

## 代码自测

### Q1: reserve vs resize
```cpp
std::vector<int> v;
v.reserve(10);   // A
std::cout << v.size() << ' ' << v.capacity() << '\n';

std::vector<int> v2;
v2.resize(10);   // B
std::cout << v2.size() << ' ' << v2.capacity() << '\n';
```
> A 和 B 的输出分别是什么？

<details>
<summary>答案</summary>

- **A**：`0 10`。`reserve` 只改变 capacity（预分配空间），不改变 size（没有元素）。
- **B**：`10 10`（或 capacity≥10）。`resize` 改变 size（新增 10 个值初始化为 0 的元素），capacity 至少等于 size。

`reserve` = 分配空间但不创建元素。`resize` = 创建/销毁元素。
</details>

### Q2: reserve 的时机
```cpp
std::vector<int> v;
for (int i = 0; i < 1000; ++i) {
    if (i == 500) v.reserve(1000);  // A: 中途 reserve
    v.push_back(i);
}
// vs
std::vector<int> v2;
v2.reserve(1000);  // B: 开头 reserve
for (int i = 0; i < 1000; ++i) v2.push_back(i);
```
> A 方案有什么问题？

<details>
<summary>答案</summary>

**A 方案**：前 500 次 `push_back` 已经触发了约 9 次扩容（2x：1→2→...→512）。中途 `reserve(1000)` 会触发一次额外的扩容（从 512→1000），但前 9 次扩容的代价已经产生了。

**B 方案**：只有 1 次分配，0 次扩容。

**教训**：`reserve` 要尽早调——在知道容量需求的第一时间就调。
</details>

### Q3: unordered_map reserve
```cpp
std::unordered_map<int, std::string> m;
// 预期插入 10000 个键值对
// A
for (int i = 0; i < 10000; ++i) m[i] = "val";

// B
m.reserve(10000);
for (int i = 0; i < 10000; ++i) m[i] = "val";
```
> A 方案发生了多少次 rehash？B 方案呢？

<details>
<summary>答案</summary>

- **A**：约 5-6 次 rehash（桶数增长策略通常是 2x：1→2→4→...→32768）。每次 rehash = 重新哈希所有已有元素 + 分配新桶数组 → 延迟尖峰。
- **B**：0 次 rehash。`reserve(10000)` 一次分配足够桶数，后续插入不 rehash。

**HFT**：`unordered_map` 启动时 `reserve` 避免热路径 rehash 尖峰。
</details>

### Q4: string reserve
```cpp
std::string s;
for (int i = 0; i < 100; ++i) {
    s += "hello";  // 每次 += 可能扩容
}
// vs
std::string s2;
s2.reserve(500);  // 100 * 5
for (int i = 0; i < 100; ++i) s2 += "hello";
```
> 两个方案的性能差异？

<details>
<summary>答案</summary>

方案 1：约 9 次扩容（2x 增长），每次扩容拷贝已有字符串。

方案 2：1 次分配，0 次扩容。

性能差距取决于字符串长度——长字符串的拷贝代价更大，`reserve` 收益更明显。HFT 中构造 FIX 消息时预 `reserve` 是标准做法。
</details>

---

## 参考与延伸

- 上一节：[Item 12-13 容量与扩容](item12-13-capacity-reserve.md)
- 下一节：[Item 15 string 实现多样性](item15-string-implementation.md)
- 回到：[第 2 章 vector 和 string](README.md)
