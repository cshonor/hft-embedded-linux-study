# 第 11 章 关联容器

关联容器与顺序容器不同，元素通过**关键字（key）**而非位置来存储和访问。

## 小节

- [核心类型](./11.1-核心类型.md)
- [pair 类型](./11.2-pair类型.md)
- [特有操作](./11.3-特有操作.md)


## 章节摘要

关联容器（按关键字而非位置存储）：`map`/`set`（有序，红黑树）、`unordered_map`/`unordered_set`（无序，哈希表）、`multimap`/`multiset`（允许重复关键字）。`pair` 类型、特有操作（`[]`/`find`/`insert`/`erase`）。

### 和 C 的区别

| C | C++ |
|---|-----|
| 手写哈希表 | `std::unordered_map` |
| 无有序映射 | `std::map`（红黑树 O(log n)） |
| 无 pair | `std::pair` |

## 章节自测

### Q1: map operator[] 副作用

```cpp
std::map<std::string, int> m;
m["apple"] = 5;
std::cout << m["banana"];  // A
std::cout << m.size();     // B
```

> A 和 B 分别输出什么？`operator[]` 有什么副作用？

<details>
<summary>答案与复习指引</summary>

**A: 0**（`"banana"` 不存在，`operator[]` 会**插入**一个默认构造的值 `0` 并返回）
**B: 2**（`m` 现在有 `"apple"` 和 `"banana"` 两个元素！）

**副作用：** `operator[]` 在 key 不存在时会自动插入（值初始化）。如果只是想查找，应该用 `find()` 或 `at()`（后者不存在时抛异常）。这个副作用在性能敏感代码中是隐患——每次查找都可能触发插入。

**复习：** → [特有操作](./11.3-特有操作.md)
</details>

### Q2: map vs unordered_map

```cpp
std::map<int, std::string> m;
std::unordered_map<int, std::string> um;
// 两者都插入 10000 个元素
for (int i = 0; i < 10000; ++i) {
    m[i] = std::to_string(i);
    um[i] = std::to_string(i);
}
```

> `map` 和 `unordered_map` 的查找复杂度分别是什么？底层实现是什么？

<details>
<summary>答案与复习指引</summary>

| | `map` | `unordered_map` |
|---|-------|-----------------|
| 底层 | 红黑树 | 哈希表 |
| 查找 | O(log n) | O(1) 均摊（最坏 O(n)） |
| 有序 | 是（按 key 排序遍历） | 否 |
| 内存 | 节点分散，cache 不友好 | 桶数组+链表，稍好 |

**HFT 场景：** 小规模/需要有序遍历用 `map`；大规模纯查找用 `unordered_map` + `reserve` 避免 rehash。

**复习：** → [核心类型](./11.1-核心类型.md)
</details>

### Q3: set 去重

```cpp
std::vector<int> v = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5};
std::set<int> s(v.begin(), v.end());
std::cout << s.size();
for (auto x : s) std::cout << " " << x;
```

> 输出是什么？`set` 和 `unordered_set` 的遍历顺序有什么不同？

<details>
<summary>答案与复习指引</summary>

**输出：** `7 1 2 3 4 5 6 9` — 去重后 7 个元素，按升序排列

**遍历顺序：**
- `set`：按 key 升序遍历（红黑树中序遍历）
- `unordered_set`：无特定顺序遍历（哈希表桶顺序）

**复习：** → [核心类型](./11.1-核心类型.md)
</details>
