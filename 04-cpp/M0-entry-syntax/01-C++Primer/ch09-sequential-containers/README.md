# 第 9 章 顺序容器

在前文（如第 3 章 `vector`）基础上，本章全面扩展 C++ 标准库中的顺序容器知识。顺序容器中元素位置与其加入容器时的顺序相对应。

## 小节

- [容器概览与操作](./9.1-容器概览与操作.md)
- [底层机制](./9.2-底层机制.md)
- [string 的额外操作](./9.3-string的额外操作.md)
- [容器适配器](./9.4-容器适配器.md)


## 章节摘要

顺序容器全览：`vector`/`deque`/`list`/`forward_list`/`array`/`string` 的选型与取舍、底层机制（连续 vs 链式）、`string` 额外操作、容器适配器（`stack`/`queue`/`priority_queue`）。

### 和 C 的区别

| C | C++ |
|---|-----|
| 手写链表 | `std::list`/`std::forward_list` |
| `malloc` 动态数组 | `std::vector`（自动扩容/析构） |
| 固定数组 | `std::array`（知道大小+边界检查 `.at()`） |
| 手写栈/队列 | `std::stack`/`std::queue` |

## 章节自测

### Q1: 容器选型

```cpp
// 场景 A: 需要随机访问，主要在尾部增删
// 场景 B: 需要在头部和尾部都增删
// 场景 C: 频繁在中间插入删除
// 场景 D: 已知大小，编译期固定
```

> A/B/C/D 分别选什么容器？

<details>
<summary>答案与复习指引</summary>

- A: `vector` — 随机访问 O(1)，尾插均摊 O(1)，cache 友好
- B: `deque` — 双端 O(1) 随机访问（分段连续，比 vector 稍慢）
- C: `list`/`forward_list` — 中间插删 O(1)（已知位置），但 cache 不友好
- D: `array` — 栈上固定大小，零开销

**HFT 默认选 `vector`** — cache 局部性是延迟敏感场景的首要因素。

**复习：** → [容器概览与操作](./9.1-容器概览与操作.md)
</details>

### Q2: vector capacity/reserve

```cpp
std::vector<int> v;
std::cout << v.size() << " " << v.capacity() << "\n";
v.push_back(1);
v.push_back(2);
v.push_back(3);
v.reserve(100);
std::cout << v.size() << " " << v.capacity();
```

> 两行输出分别是什么？

<details>
<summary>答案与复习指引</summary>

**第一行：** `0 0`（空 vector，size 和 capacity 都是 0）
**第二行：** `3 100`（3 个元素，reserve 到 100 后 capacity = 100，但 size 仍为 3）

**`reserve` 的意义：** 预分配 capacity，避免 `push_back` 时多次扩容（每次扩容拷贝所有元素）。HFT 启动时按峰值 `reserve`。

**`size` vs `capacity`：** size = 实际元素数；capacity = 已分配的底层内存能容纳的元素数。

**复习：** → [底层机制](./9.2-底层机制.md)
</details>

### Q3: erase-remove 惯用法

```cpp
std::vector<int> v = {1, 2, 3, 2, 4, 2, 5};
v.erase(std::remove(v.begin(), v.end(), 2), v.end());
// v 现在是什么？
```

> v 是什么？为什么不直接 `v.erase(it)` 遍历删除？

<details>
<summary>答案与复习指引</summary>

**v = `{1, 3, 4, 5}`** — 删除了所有 2。

**erase-remove 惯用法：**
- `remove` 把不等于 2 的元素移到前面，返回新的逻辑终点迭代器
- `erase` 删除从新终点到旧终点之间的"垃圾"元素

**不直接遍历 erase 的原因：**
1. 遍历中 erase 会使迭代器失效——`it = v.erase(it)` 才安全，但容易写错
2. 每次 erase 是 O(n) 移动，遍历 erase 总共 O(n²)；erase-remove 是 O(n)

**复习：** → [容器概览与操作](./9.1-容器概览与操作.md)
</details>

### Q4: array vs C 数组

```cpp
std::array<int, 5> arr = {1, 2, 3, 4, 5};
int carr[5] = {1, 2, 3, 4, 5};
// arr.size()  // A: 合法？
// carr.size() // B: 合法？
// arr.at(10)  // C: 会怎样？
```

> A、B、C 分别怎样？

<details>
<summary>答案与复习指引</summary>

- A: 合法，返回 5。`std::array` 是有接口的容器（`size()`/`begin()`/`end()`/`at()`）
- B: 编译错误。C 数组没有成员函数
- C: 抛 `std::out_of_range` 异常。`at()` 做边界检查；`operator[]` 不检查（越界是 UB）

**`std::array` 优势：** 知道自己的大小、支持迭代器/算法、可传值/传引用不退化为指针、零开销（和 C 数组一样的内存布局）。

**复习：** → [容器概览与操作](./9.1-容器概览与操作.md)
</details>
