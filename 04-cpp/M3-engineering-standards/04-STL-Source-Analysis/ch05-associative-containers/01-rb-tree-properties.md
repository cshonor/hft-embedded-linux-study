# 5.1 红黑树的五大性质
> 第 5 章 关联容器 · 第 1 节 · 上一节：[本章概览](README.md) · 下一节：[5.2 set/map 封装](02-set-map-as-rb-tree.md)

## 为什么要学这个（先建立直觉）

C 里没有红黑树——你得自己手写或用 glibc 的 `tsearch`（不透明、难调试）：

```c
// C: 手写平衡树或用 glibc tsearch
#include <search.h>
void *root = NULL;
tsearch(&key, &root, cmp);  // 不透明，无法遍历
// twalk / tfind / tdelete，API 极其难用
```

C++ 的 `map`/`set` 内置红黑树，接口干净、类型安全：

```cpp
std::map<int, std::string> m;
m[42] = "answer";
auto it = m.find(42);  // O(log n)，类型安全
for (auto& [k, v] : m) { /* 有序遍历 */ }
```

理解红黑树性质，你才能理解 `map` 的性能边界：为什么插入是 O(log n) 而非 O(1)。

## 这节讲什么

红黑树是一种**自平衡二叉搜索树**，通过 5 条性质约束树高 ≤ 2·log(n+1)，保证查找/插入/删除均为 O(log n)。

### 红黑树五大性质

| 性质 | 描述 | 作用 |
|------|------|------|
| ① | 节点非红即黑 | 基本约束 |
| ② | 根节点是黑色 | 防止根红导致上溢 |
| ③ | 叶子（NIL 哨兵）是黑色 | 统一边界处理 |
| ④ | 红节点的子节点必黑（不能连续红） | 限制路径上红节点密度 |
| ⑤ | 任一节点到其叶的所有路径，黑节点数相同（黑高） | 保证最长路径 ≤ 2×最短路径 |

性质 ④ + ⑤ 合力保证：**最长路径（红黑交替）≤ 2 × 最短路径（全黑）**，因此树高 ≤ 2·log(n+1)。

### 旋转与着色

插入/删除可能破坏性质，通过旋转 + 着色恢复：

```
// 左旋（简化示意）
//     P                  R
//    / \                / \
//   L   R      →      P   RR
//      / \            / \
//    RL  RR          L  RL

void __rb_tree_rotate_left(__rb_tree_node_base* x) {
    __rb_tree_node_base* y = x->right;   // R
    x->right = y->left;                  // RL 成为 P 的右子
    if (y->left) y->left->parent = x;
    y->parent = x->parent;               // R 接管 P 的父
    if (!x->parent) root() = y;
    else if (x == x->parent->left) x->parent->left = y;
    else x->parent->right = y;
    y->left = x;                         // P 成为 R 的左子
    x->parent = y;
}
```

- **插入**：新节点标红，若父也红则违反 ④，需调整（最多 2 次旋转）
- **删除**：删黑节点导致黑高不等，需调整（最多 3 次旋转）
- 旋转次数有上限，保证 O(log n) 的调整代价

### 为什么选红黑树而非 AVL 树？

| 树 | 平衡条件 | 查找 | 插入/删除旋转 | 适用场景 |
|----|---------|------|-------------|---------|
| AVL | 高度差 ≤ 1 | 最快 O(log n) | 可能 O(log n) 次 | 查找密集 |
| 红黑树 | 黑高相等 | 略慢 | 最多 2/3 次 | 插入删除频繁 |

`map`/`set` 的典型使用模式是频繁增删，红黑树旋转更少，综合更优。

## 常见错误（新手踩坑）

### 错误 1：以为红黑树查找是 O(1)

```cpp
// ❌ 错误假设：map 查找和 unordered_map 一样快
std::map<int, int> m;
auto it = m.find(key);  // O(log n)，不是 O(1)
```

红黑树查找是 O(log n)——从根到叶走一条路径。`unordered_map` 才是平均 O(1)。

### 错误 2：以为遍历 map 是无序的

```cpp
// ❌ 错误：以为 map 遍历顺序和插入顺序一致
std::map<int, string> m;
m[3] = "c"; m[1] = "a"; m[2] = "b";
for (auto& [k, v] : m) cout << v;  // 输出 "abc"（按 key 升序，不是 "cab"）
```

红黑树是**二叉搜索树**，中序遍历有序。遍历 `map` 总是按键升序。

### 错误 3：修改 map 的键

```cpp
// ❌ 编译错误：map 的键是 const
std::map<int, string> m;
auto it = m.find(1);
it->first = 2;  // 错误！pair<const int, string> 的 first 是 const
```

`map` 的元素是 `pair<const Key, Value>`，键不可修改。需要改键只能先 erase 再 insert。

## 新手要点（和 C 的区别）

| C | C++ | 区别 |
|----|-----|------|
| 手写 BST 或用 `tsearch` | `std::map`/`set` 内置 | C++ 红黑树零手写 |
| 无类型安全（`void*`） | 模板，编译期类型检查 | C 的 `tsearch` 全是 `void*` |
| 手动平衡或退化成链表 | 自动平衡 O(log n) | C 的 BST 可能退化为 O(n) |
| 无有序遍历保证 | 中序遍历保证有序 | C++ `map` 遍历即排序 |

## HFT 关联

- **订单簿价格档位**：最优买卖价需要有序范围查询，红黑树 `map<Price, Volume>` 是自然选择；但 `std::map` 节点堆分配 + 指针追逐 cache 不友好，HFT 常自建连续存储的红黑树
- **插入删除频繁**：订单不断增删，红黑树旋转少（最多 3 次）比 AVL 更适合高变动场景
- **节点大小**：每个 `map` 节点 3 指针 + 颜色 + 数据 ≈ 60+ 字节，内存分散——热路径考虑 `vector<pair>` + `lower_bound` 替代

## 代码自测

### Q1: 红黑树为什么能保证 O(log n)？

```cpp
// 红黑树五大性质的核心是"最长路径 ≤ 2 × 最短路径"
// 性质④：红节点子必黑 → 不能连续红
// 性质⑤：黑高相等 → 所有路径黑节点数相同
```
> 如果一棵红黑树有 n 个内部节点，树高最大是多少？为什么？

<details>
<summary>答案与复习指引</summary>

**最大树高 ≤ 2·log₂(n+1)**：

- 最短路径全黑，长度 = 黑高 bh
- 最长路径红黑交替，长度 = 2·bh（性质 ④ 保证红黑交替最多翻倍）
- n ≥ 2^bh - 1（至少 bh 层黑节点），所以 bh ≤ log₂(n+1)
- 树高 ≤ 2·bh ≤ 2·log₂(n+1)

因此查找/插入/删除走根到叶路径，均为 O(log n)。

**复习：** → [红黑树五大性质](./01-rb-tree-properties.md)
</details>

### Q2: 插入新节点为什么标红？

```cpp
// SGI STL 插入
__rb_tree_node* z = create_node(value);
z->color = __rb_tree_red;  // 新节点标红
// 然后根据父节点颜色决定是否调整
```
> 新节点标红可能违反哪条性质？标黑呢？

<details>
<summary>答案与复习指引</summary>

**标红**：只可能违反性质 ④（父也红 → 连续红）。如果父黑，无需任何调整。

**标黑**：一定违反性质 ⑤（黑高增加，所有路径的黑节点数不再相等），且调整更复杂。

所以标红是"最不破坏平衡"的选择——只有在父也是红色时才需要修复。

**修复策略**：
- 叔叔红 → 父和叔变黑、祖变红（上溢，递归处理祖）
- 叔叔黑 → 旋转 + 着色（最多 2 次旋转搞定）

**复习：** → [旋转与着色](./01-rb-tree-properties.md)
</details>

### Q3: 下面的代码有什么问题？

```cpp
std::map<int, std::string> orders;
orders[100] = "buy";
orders[50] = "sell";
orders[200] = "buy";

// 想找价格 >= 100 的第一笔
auto it = std::find(orders.begin(), orders.end(), 100);
```
> std::find 在 map 上是什么复杂度？应该用什么？

<details>
<summary>答案与复习指引</summary>

**问题**：`std::find` 对 `map` 是 **O(n)** 线性扫描——它不利用红黑树的有序性。

**正确做法**：用 `orders.lower_bound(100)`，O(log n) 返回第一个 key ≥ 100 的迭代器。

```cpp
auto it = orders.lower_bound(100);  // O(log n)
if (it != orders.end()) {
    std::cout << it->second;  // 第一个 >= 100 的订单
}
```

**复习：** → 成员函数 vs 算法（Effective STL Item 46）
</details>

### Q4: 删除黑节点会发生什么？

```cpp
std::map<int, int> m;
// ... 插入若干节点 ...
m.erase(some_black_key);  // 删除一个黑节点
```
> 删除黑节点为什么需要"双重黑"修复？

<details>
<summary>答案与复习指引</summary>

删除黑节点会导致**经过该节点的路径黑高减 1**，违反性质 ⑤。

**修复策略**（SGI 实现）：
- 给替代节点加"额外黑色"（双重黑或红黑）
- 通过旋转和着色把额外黑色向上推直到消除
- 最多 3 次旋转恢复平衡

删除红节点不影响黑高，无需修复。

**HFT**：map 的 erase 不是 O(1) 而是 O(log n)（含调整），热路径批量删除时注意延迟。

**复习：** → [红黑树删除修复](./01-rb-tree-properties.md)
</details>

## 参考与延伸

- 上一节：[本章概览](README.md)
- 下一节：[5.2 set/map 封装](02-set-map-as-rb-tree.md)
- 源码参考：`/usr/include/c++/*/bits/stl_tree.h`（GCC libstdc++ 的 `__rb_tree` 实现）
