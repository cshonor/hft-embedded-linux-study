# 第 5 章 算法

**Algorithms** — Items 32–37

## 本章讲什么

STL 算法是"意图表达"的利器——用 `sort`/`find`/`copy` 比手写循环更清晰且不易错。本章讲最经典的 `remove`-`erase` 惯用法、算法相对手写循环的优势、以及如何选对算法。

---

## 各 Item 要点

### Item 32–33：`remove` + `erase` 惯用法（核心）

**`remove` 不删除元素**——它只是把"不该删的"前移，返回新逻辑终点迭代器，容器 size 不变。真正删除要配合 `erase`：

```cpp
std::vector<int> v = {1,2,3,2,4};
v.erase(std::remove(v.begin(), v.end(), 2), v.end());  // {1,3,4}
// C++20: std::erase(v, 2);  更简洁
```

`remove` 的本质是"覆盖式前移"——被删元素被后面的覆盖，末尾留下"垃圾"（未指定值），`erase` 砍掉尾巴。

**指针容器陷阱**：`remove` 跳过的指针会被覆盖丢失（泄漏），删除前要 `for_each` 先 `delete`。或存智能指针让 `erase` 自动析构。

### Item 34：了解排序后用 `binary_search` / `lower_bound`

有序区间上 `binary_search`/`lower_bound`/`upper_bound` 是 O(log n)。`std::sort` + `binary_search` 比 `find`（O(n)）快——但要先排序，且每次插入维持有序有代价。静态数据排序后二分；动态数据用 `set`/`unordered_set`。

### Item 35：算法通常优于手写循环

- **正确性**：算法经过充分测试，手写循环易 off-by-one。
- **可读性**：`std::copy_if` 比 `for + if + push_back` 更直白。
- **性能**：算法能针对迭代器分类特化（如 `copy` 对 `memmove` 特化）。

例外：当算法需要复杂的绑定/适配器才能表达时，简单循环可能更清晰。C++14 lambda + 算法的组合已大幅减少这种例外。

### Item 36：正确选择算法

`for_each` vs `transform` vs `copy`：`for_each` 原地修改不产生新区间；`transform` 产生新区间；`copy` 仅搬运。按意图选，别混用。

`sort` vs `stable_sort` vs `partial_sort` vs `nth_element`：
- `sort`：全序 O(n log n)，不保相对顺序。
- `stable_sort`：保相等元素相对顺序 O(n log²n)。
- `partial_sort`：只排前 k 个 O(n log k)。
- `nth_element`：只保证第 n 大在位 O(n)，不完全排序。

### Item 37：`accumulate` / `for_each` / `fill` 的语义

`accumulate` 默认求和，可挂自定义二元操作（积、拼接）；`for_each` 遍历执行可变操作；`fill` 填充。`accumulate` 的初值类型决定结果类型——`accumulate(v.begin(), v.end(), 0)` 当 `int` 算（溢出风险），用 `0LL` 当 `long long`。

---

## HFT 关联

- **`remove`-`erase` 与订单清理**：撤单后清理无效档位用 `erase(remove_if(...))`，但注意指针/迭代器失效——先收集要删的，批量 `erase`。
- **`nth_element` 求分位数**：回测里求 tick 序列的 P99 延迟，`nth_element` O(n) 比全排序 O(n log n) 快。
- **`accumulate` 初值类型陷阱**：累加成交额用 `0LL`（int64）而非 `0`（int），避免大额溢出——和定点价格铁律一致。
- **`copy` 的 `memmove` 特化**：连续内存容器间 `copy` 被特化为 `memmove`，HFT 批量拷贝 tick 用 `std::copy` 比手写循环更可能走 SIMD/`memmove`。

---

## 自测题

1. `remove` 为什么不真正删除元素？`erase` 在惯用法中砍掉的是什么？
2. 指针容器直接 `remove`+`erase` 会发生什么？正确做法是什么？
3. `sort`/`stable_sort`/`partial_sort`/`nth_element` 各自的时间复杂度与适用场景？
4. `accumulate(v.begin(), v.end(), 0)` 与 `accumulate(v.begin(), v.end(), 0LL)` 结果类型有何不同？为什么 HFT 要用后者？
5. 为什么 `std::copy` 在连续内存容器间可能比手写循环快？
