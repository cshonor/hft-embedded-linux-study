# Item 12-13：理解 vector/string 的容量与扩容

> 第 2 章 vector 和 string · Item 12-13 · 下一节：[Item 14 reserve 避免重新分配](item14-reserve-avoid-realloc.md)

## 为什么要学这个（先建立直觉）

C 程序员动态数组的扩容是手动的：

```c
int* arr = malloc(10 * sizeof(int));
int capacity = 10, size = 0;
// 满了就手动扩容
if (size == capacity) {
    capacity *= 2;
    arr = realloc(arr, capacity * sizeof(int));  // 可能搬移整块内存
}
arr[size++] = 42;
```

C++ 的 `vector` 自动管理扩容，但扩容时迭代器/指针/引用全部失效——这是 `vector` 最常见的 UB 来源。

```cpp
std::vector<int> v;
v.push_back(1);
auto p = &v[0];      // p 指向 v 的数据
v.push_back(2);       // 可能扩容 → p 悬空！
// *p;  // UB！
```

---

## 这节讲什么

`vector` 的 `size()`/`capacity()`/`reserve()`/`shrink_to_fit()` 四件套。扩容策略通常是翻倍——均摊 O(1) 插入，但单次扩容有 O(n) 拷贝/移动尖峰。扩容时所有迭代器、指针、引用全部失效。

---

## 容量四件套

```cpp
std::vector<int> v;
v.push_back(1);  // size=1, capacity 可能=1

v.reserve(100);  // capacity≥100, size 不变
// 预分配 100 个元素的空间，后续 push_back 不扩容

v.shrink_to_fit();  // 请求 capacity=size（非强制）
// 减少内存占用，但实现可以忽略

std::cout << v.size() << ' ' << v.capacity();
// size = 当前元素数, capacity = 已分配可容纳数
```

### 扩容过程

```cpp
std::vector<int> v;
for (int i = 0; i < 10; ++i)
    v.push_back(i);

// 典型扩容轨迹（GCC libstdc++ 2x 策略）：
// push_back(0): size=1, capacity=1   → 分配 1
// push_back(1): size=2, capacity=2   → 分配 2，拷贝 1 个
// push_back(2): size=3, capacity=4   → 分配 4，拷贝 2 个
// push_back(3): size=4, capacity=4   → 无扩容
// push_back(4): size=5, capacity=8   → 分配 8，拷贝 4 个
// ...
// push_back(9): size=10, capacity=16 → 分配 16，拷贝 8 个
// 总分配次数：4-5 次，总拷贝次数：1+2+4+8=15 次
```

### 迭代器失效规则

| 操作 | 失效范围 |
|------|---------|
| `push_back` | 扩容→全部失效；不扩容→`end()` 失效 |
| `insert(pos, val)` | pos 之后全部失效（扩容则全部） |
| `erase(pos)` | pos 及之后失效 |
| `reserve/clear` | 全部失效 |
| `shrink_to_fit` | 全部失效 |

---

## 常见错误（新手踩坑）

### 错误 1：扩容后使用旧指针/迭代器

```cpp
std::vector<int> v = {1, 2, 3};
auto it = v.begin();
v.push_back(4);  // 可能扩容 → it 失效
std::cout << *it;  // UB！
```

**修正：** 扩容后重新获取迭代器，或预 `reserve` 避免扩容。

### 错误 2：热路径上动态扩容导致延迟尖峰

```cpp
// HFT 热路径：每 tick 都 push_back，可能触发扩容
for (auto& tick : incoming_ticks)
    tick_buffer.push_back(tick);  // 扩容时 O(n) 拷贝 → 延迟尖峰
```

**修正：** 启动时 `tick_buffer.reserve(MAX_TICKS);` 消除热路径扩容。

### 错误 3：shrink_to_fit 后假设容量

```cpp
std::vector<int> v;
v.reserve(1000);
v.push_back(1);
v.shrink_to_fit();
// v.capacity() 可能是 1，也可能不是——标准不保证
```

**修正：** `shrink_to_fit` 是非绑定请求，不要依赖其效果。检查 `capacity()` 确认。

---

## 新手要点（和 C 的区别）

| 维度 | C | C++ STL | 为什么 |
|------|---|---------|--------|
| 动态数组 | `malloc`/`realloc` | `vector` 自动扩容 | 异常安全 |
| 容量管理 | 手动 `capacity` 变量 | `size()`/`capacity()` | 内建 |
| 预分配 | `realloc` 手动 | `reserve()` | 语义清晰 |
| 扩容失效 | `realloc` 后旧指针失效 | 迭代器/指针/引用失效 | 相同问题 |
| 收缩 | `realloc` 缩小 | `shrink_to_fit()`（非强制） | 标准不保证 |

**一句话：** C 的 `realloc` 和 `vector` 扩容本质相同——都可能搬移内存。`reserve` 是 C++ 版的"提前 `realloc` 到足够大小"，消除热路径上的扩容尖峰。

---

## HFT 关联

- **`reserve` 消除扩容尖峰**：行情 tick 缓冲按峰值 `reserve`，热路径零扩容。这是 HFT `vector` 调优第一课。
- **`unordered_map::reserve` 避免 rehash**：`reserve(bucket_count)` 预分配桶数组，避免热路径 rehash。
- **迭代器失效是 UB**：热路径上保存的指针/迭代器在扩容后变悬空——预 `reserve` 或用索引代替指针。

---

## 代码自测

### Q1: 扩容代价
```cpp
std::vector<int> v;
for (int i = 0; i < 1000; ++i) v.push_back(i);  // A: 无 reserve

std::vector<int> v2;
v2.reserve(1000);
for (int i = 0; i < 1000; ++i) v2.push_back(i);  // B: 有 reserve
```
> A 和 B 各发生多少次内存分配？

<details>
<summary>答案</summary>

- **A**：约 10 次分配（2x 增长：1→2→4→...→1024，log₂(1000)≈10）。每次分配 = 新内存 + 拷贝旧元素 + 释放旧内存。
- **B**：1 次分配（`reserve(1000)` 一次分配，后续 `push_back` 不扩容）。
</details>

### Q2: 迭代器失效
```cpp
std::vector<int> v = {1, 2, 3};
auto it = v.begin() + 1;  // 指向 2
v.reserve(100);           // A
v.push_back(4);           // B
std::cout << *it;         // C
```
> C 行安全吗？

<details>
<summary>答案</summary>

**视情况**：
- 如果 `reserve(100)` 扩容了（原 capacity < 100）→ A 行后 `it` 已失效 → C 行 UB。
- 如果 `reserve(100)` 没扩容（原 capacity ≥ 100）→ A 行后 `it` 有效 → B 行不扩容（capacity 足够）→ C 行安全。

**最佳实践**：`reserve` 后不要使用旧迭代器——重新获取。
</details>

### Q3: capacity 增长策略
```cpp
std::vector<int> v;
for (int i = 0; i < 20; ++i) {
    v.push_back(i);
    std::cout << v.capacity() << ' ';
}
```
> GCC libstdc++ 和 MSVC 的输出有何不同？

<details>
<summary>答案</summary>

- **GCC libstdc++（2x）**：1 2 4 8 16 16 16 16 32 ...（翻倍增长）
- **MSVC（1.5x）**：1 2 3 4 6 9 13 19 28 ...（1.5 倍增长）

倍率越大→扩容次数越少但浪费越多。倍率越小→扩容次数多但内存利用率高。`reserve(n)` 让你自己控制——HFT 按 peak 预分配。
</details>

### Q4: shrink_to_fit
```cpp
std::vector<int> v;
v.reserve(1000);
for (int i = 0; i < 10; ++i) v.push_back(i);
std::cout << v.capacity() << ' ';  // A
v.shrink_to_fit();
std::cout << v.capacity();          // B
```
> A 输出什么？B 一定小于 A 吗？

<details>
<summary>答案</summary>

- **A**：1000（`reserve` 的值）。
- **B**：不一定小于 1000。`shrink_to_fit` 是非绑定请求——标准说"不保证收缩"。大多数实现会收缩到 size（10），但不是强制的。

不要依赖 `shrink_to_fit` 的行为。如果必须释放内存，用 `vector<int>().swap(v);`（swap 技巧强制释放）。
</details>

---

## 参考与延伸

- 下一节：[Item 14 reserve 避免重新分配](item14-reserve-avoid-realloc.md)
- 回到：[第 2 章 vector 和 string](README.md)
