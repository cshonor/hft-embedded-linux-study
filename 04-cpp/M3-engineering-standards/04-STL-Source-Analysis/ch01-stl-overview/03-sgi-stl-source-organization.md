# 1.3 SGI STL 源码组织

> 第 1 章 STL 概览 · 第 3 节 · 上一节：[1.2 泛型编程](02-generic-programming.md) · 下一节：[第 2 章 空间配置器](../ch02-allocator/README.md)

## 为什么要学这个（先建立直觉）

在 C 里，标准库源码在 glibc 中，文件名直观（`stdio.h`→`printf`）。C++ STL 的源码组织更复杂——SGI STL（侯捷剖析的版本）按组件拆分，文件名有前缀约定。

```c
/* C: glibc 源码组织 */
// /usr/include/stdio.h  → printf, fopen
// /usr/include/stdlib.h → malloc, exit
// /usr/include/string.h → strlen, memcpy
// 文件名 = 头文件名，直觉清晰
```

```cpp
// C++: SGI STL 源码组织
// <stl_alloc.h>    → allocator（空间配置器）
// <stl_vector.h>   → vector 实现
// <stl_list.h>     → list 实现
// <stl_algo.h>     → 算法实现
// <stl_iterator.h> → 迭代器与 traits
// <stl_function.h> → 仿函数与适配器
// <stl_tree.h>     → RB-tree（map/set 底层）
// <stl_hashtable.h>→ hashtable（unordered_* 底层）
```

**直觉**：SGI STL 按组件类型分文件，`stl_` 前缀标识内部实现头。现代标准库（libstdc++/libc++）文件名不同但组织方式类似。

## 这节讲什么

### SGI STL vs 标准 C++ 头文件

| SGI 内部头 | 标准 C++ 头 | 内容 |
|-----------|------------|------|
| `<stl_alloc.h>` | `<memory>` | allocator |
| `<stl_vector.h>` | `<vector>` | vector |
| `<stl_list.h>` | `<list>` | list |
| `<stl_deque.h>` | `<deque>` | deque |
| `<stl_tree.h>` | `<map>`/`<set>` | RB-tree |
| `<stl_hashtable.h>` | `<unordered_map>`/`<unordered_set>` | hashtable |
| `<stl_algo.h>` | `<algorithm>` | 算法 |
| `<stl_iterator.h>` | `<iterator>` | 迭代器 |
| `<stl_function.h>` | `<functional>` | 仿函数 |

### 侯捷为什么选 SGI STL

1. **SGI STL 是 GNU C++ 标准库的基础**：GCC 的 libstdc++ 基于 SGI STL 演化
2. **源码可读性好**：SGI STL 代码风格清晰，注释丰富
3. **结构代表性**：六大组件的源码组织清晰，适合教学

### 现代 libstdc++ 源码位置

```bash
# GCC libstdc++ 源码（Linux）：
/usr/include/c++/<version>/vector           # 公开头文件
/usr/include/c++/<version>/bits/stl_vector.h  # 实现头
/usr/include/c++/<version>/bits/stl_algo.h    # 算法实现
/usr/include/c++/<version>/bits/stl_tree.h    # RB-tree
/usr/include/c++/<version>/bits/hashtable.h   # hashtable
```

### 读源码的入口顺序

```
1. <vector> → stl_vector.h → 看三指针 (start/finish/end_of_storage)
2. <list> → stl_list.h → 看 node 结构和环形链表
3. <algorithm> → stl_algo.h → 看 sort 的 introsort
4. <iterator> → stl_iterator.h → 看 traits 偏特化
5. <map> → stl_tree.h → 看红黑树节点和旋转
```

## 常见错误（新手踩坑）

### 错误 1：在代码中直接包含 SGI 内部头

```cpp
#include <stl_vector.h>  // 不要这样做！
// 应该用标准头文件
#include <vector>
```

### 错误 2：以为侯捷的书内容已过时

SGI STL 的设计思想（六大组件、traits、迭代器分类）在现代 C++ 中仍然适用。变化的主要是语法细节（C++11 lambda 替代手写仿函数等）。

### 错误 3：混淆 SGI hash_map 和标准 unordered_map

```cpp
#include <hash_map>              // SGI 扩展，非标准
#include <unordered_map>         // C++11 标准
// hash_map 是 unordered_map 的前身
```

## 新手要点（和 C 的区别）

| 方面 | C (glibc) | C++ (STL) |
|------|-----------|-----------|
| 源码组织 | 按功能域（stdio/stdlib） | 按组件类型（stl_vector/stl_algo） |
| 内部头 | 少（大部分公开） | 多（bits/ 下的实现头） |
| 读源码入口 | 直接看 .h 文件 | 先看公开头，再跟到 bits/ 实现 |

## HFT 关联

- **读 libstdc++ 源码理解实现**：vector 扩容因子（GCC 2x）、unordered_map 哈希策略，做精确性能预估
- **godbolt 验证**：在 Compiler Explorer 上看 STL 源码展开后的汇编，验证内联
- **自定义容器遵循 STL 接口**：HFT 自建容器提供 `begin()`/`end()`/迭代器，才能复用 STL 算法

## 代码自测

### Q1: 源码定位

```
你想看 std::sort 的实现代码。
在 GCC libstdc++ 中，应该看哪个文件？
```

<details>
<summary>答案</summary>

看 `/usr/include/c++/<version>/bits/stl_algo.h`。

`<algorithm>` 是公开头文件，它 `#include` 了 `bits/stl_algo.h`，实际实现在后者中。

在 SGI STL 中对应 `<stl_algo.h>`。
</details>

### Q2: SGI vs 标准

```
SGI STL 的 hash_map 在 C++11 中标准化为什么？
```

<details>
<summary>答案</summary>

`std::unordered_map`。

SGI 的 `hash_map`/`hash_set` 是非标准扩展。C++11 标准化了哈希容器，改名为 `unordered_map`/`unordered_set`（因为 "hash" 这个名字被太多库占用了）。

接口基本一致，但 `unordered_map` 的 API 更完善（如 `bucket_count()`、`load_factor()`、`rehash()` 等）。
</details>

### Q3: 组件到源文件映射

```
map 的底层是红黑树。
在 SGI STL 中，红黑树的实现在哪个头文件？
```

<details>
<summary>答案</summary>

`<stl_tree.h>`。

`<map>` 和 `<set>` 都内部包含 `<stl_tree.h>`，后者定义了 `__rb_tree` 类模板。

- `set<T>` = `rb_tree<T, T, identity<T>, less<T>>`（键值合一）
- `map<K,V>` = `rb_tree<pair<const K,V>, ...>`

在 GCC libstdc++ 中对应 `bits/stl_tree.h`。
</details>

### Q4: 读源码的价值

```
为什么要读 STL 源码？不能只看文档吗？
```

<details>
<summary>答案</summary>

文档告诉你"是什么"和"复杂度保证"，源码告诉你"怎么做到的"：

1. **vector 扩容因子**：文档说"capacity 增加"，源码告诉你 GCC 是 2x、MSVC 是 1.5x
2. **sort 用什么算法**：文档说 O(n log n)，源码告诉你用 introsort（快排+堆排+插入排序）
3. **copy 何时走 memmove**：文档说"可能优化"，源码告诉你 trivially copyable + random_access 时特化
4. **unordered_map 哈希函数**：文档说 O(1) 平均，源码告诉你桶数是素数、负载因子阈值 1.0

**HFT**：源码级理解让你能精确预测延迟、选择最优容器、避免性能悬崖。
</details>

## 参考与延伸

- 上一节：[1.2 泛型编程](02-generic-programming.md)
- 下一节：[第 2 章 空间配置器](../ch02-allocator/README.md)
