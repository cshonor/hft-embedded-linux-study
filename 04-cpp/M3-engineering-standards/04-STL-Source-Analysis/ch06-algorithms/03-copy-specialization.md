# 6.3 copy 的特化优化
> 第 6 章 算法 · 第 3 节 · 上一节：[6.2 二分查找算法](02-binary-search-algorithms.md) · 下一节：[6.4 accumulate/for_each](04-accumulate-foreach.md)

## 为什么要学这个（先建立直觉）

C 里拷贝数组用 `memcpy`/`memmove`，极快但只适用于 trivial 类型：

```c
// C: memcpy 只管字节，不管类型
int src[100], dst[100];
memcpy(dst, src, 100 * sizeof(int));  // 快，但不安全
// 如果是带析构函数的类型，memcpy 会泄漏/崩溃
```

C++ 的 `std::copy` 安全地逐元素赋值，但对 trivial 类型会自动特化为 `memmove`：

```cpp
std::copy(src.begin(), src.end(), dst.begin());  // 安全且快速
// 对 int/double 等 trivially copyable 类型，编译器特化为 memmove
// 对带构造/析构的类型，逐元素调用 operator=
```

理解这个特化机制，你才能理解为什么 `std::copy` 比手写循环更可能被优化。

## 这节讲什么

`std::copy` 通过 traits 萃取类型信息和迭代器分类，在编译期选择最优拷贝策略。

### 特化决策链

```
std::copy(first, last, result)
  │
  ├─ iterator_category == random_access?
  │    └─ trivially copyable?
  │         └─ YES → memmove(result, first, n * sizeof(T))  ← 最快路径
  │         └─ NO  → 逐元素 assignment
  │
  └─ 非 random_access → 逐元素 assignment
```

### 源码分派

```cpp
// SGI copy 的分派（简化）
template<class InputIterator, class OutputIterator>
OutputIterator copy(InputIterator first, InputIterator last,
                    OutputIterator result) {
    return __copy_dispatch<InputIterator, OutputIterator>::
        copy(first, last, result);
}

// __copy_dispatch 根据迭代器类型和 value_type 分派
template<class T>
T* copy(const T* first, const T* last, T* result) {
    // 1. 检查是否 trivially copyable（has_trivial_assignment_operator）
    // 2. 如果是 → memmove
    // 3. 如果不是 → 逐元素赋值
}
```

### memmove vs memcpy

```cpp
// memmove 处理重叠区域（安全）
// memcpy 不处理重叠（更快但不安全）
// STL copy 用 memmove（因为源和目标可能重叠）
void* memmove(void* dest, const void* src, size_t n);
```

`std::copy` 选 `memmove` 而非 `memcpy`，因为 `copy_backward` 可能导致源目标重叠，`memmove` 更安全。

### trivially copyable 判断

```cpp
// C++11: std::is_trivially_copyable<T>::value
// true:  int, double, float, 指针, POD struct
// false: std::string（有析构），std::vector（有析构），带虚函数的类

static_assert(std::is_trivially_copyable_v<int>);           // true
static_assert(!std::is_trivially_copyable_v<std::string>);  // false（有析构）
```

### 特化的效果

```cpp
// 对 int 数组，copy 特化为 memmove
int src[1000], dst[1000];
std::copy(src, src + 1000, dst);
// 编译后等价于: memmove(dst, src, 4000);  ← 极快

// 对 string 数组，copy 逐元素赋值
std::string src[1000], dst[1000];
std::copy(src, src + 1000, dst);
// 逐元素调用 operator=（可能涉及深拷贝）
```

## 常见错误（新手踩坑）

### 错误 1：手写循环替代 copy

```cpp
// ❌ 手写循环可能阻止 memmove 优化
for (int i = 0; i < n; ++i)
    dst[i] = src[i];
// 编译器可能优化为 memcpy，但不保证
// std::copy 明确特化为 memmove，更可靠

// ✅ 用 std::copy
std::copy(src, src + n, dst);  // 保证 memmove
```

### 错误 2：源目标重叠用 copy

```cpp
// ❌ std::copy 不保证处理重叠区域
int arr[10] = {1,2,3,4,5,6,7,8,9,10};
std::copy(arr, arr + 8, arr + 2);  // 未定义行为！
// 源 [0,8) 和目标 [2,10) 重叠

// ✅ 重叠用 copy_backward 或 memmove
std::copy_backward(arr, arr + 8, arr + 10);  // 安全
```

### 错误 3：目标空间不足

```cpp
// ❌ copy 不会自动扩容目标
std::vector<int> src = {1,2,3,4,5};
std::vector<int> dst;  // 空！
std::copy(src.begin(), src.end(), dst.begin());  // 越界！未定义行为

// ✅ 预分配或用 back_inserter
std::vector<int> dst1(src.size());
std::copy(src.begin(), src.end(), dst1.begin());  // OK

std::vector<int> dst2;
std::copy(src.begin(), src.end(), std::back_inserter(dst2));  // OK，自动扩容
```

## 新手要点（和 C 的区别）

| C | C++ | 区别 |
|----|-----|------|
| `memcpy`/`memmove` 只管字节 | `std::copy` 自动判断类型 | C++ 安全 |
| 手动判断类型选拷贝方式 | traits 编译期自动特化 | C++ 零开销 |
| 不安全（对非 trivial 类型崩溃） | 安全（对非 trivial 逐元素赋值） | C++ 正确 |
| 无迭代器抽象 | 适用于任何迭代器 | C++ 泛型 |

## HFT 关联

- **批量拷贝 tick 数据**：`std::copy(ticks, ticks + n, buffer)` 对 `Tick`（POD struct）特化为 `memmove`/SIMD，比手写循环更可能被优化
- **memmove 特化**：trivially copyable 类型（订单 struct 无析构函数）自动走 `memmove`，零开销
- **back_inserter 陷阱**：`copy` + `back_inserter` 每元素 `push_back` 可能多次扩容——HFT 先 `reserve` 再 `copy` 到 `begin()` 或用 `insert` 区间版

## 代码自测

### Q1: std::copy 在什么条件下特化为 memmove？

```cpp
std::vector<int> src = {1,2,3,4,5};
std::vector<int> dst(5);
std::copy(src.begin(), src.end(), dst.begin());
// 这里走 memmove 吗？
```
> 需要满足哪两个条件？

<details>
<summary>答案与复习指引</summary>

**两个条件**：
1. **迭代器是随机访问**（`vector`/`array`/原生指针是随机访问；`list`/`map` 不是）
2. **元素类型 trivially copyable**（`int`/`double`/POD struct 是；`string`/`vector` 不是）

`int` + `vector` → 两个条件都满足 → 走 `memmove`。

`std::string` + `vector` → 不满足条件 2 → 逐元素 `operator=`。

`int` + `list` → 不满足条件 1 → 逐元素赋值（虽然 `int` 是 trivially copyable，但链表不是连续内存，无法 memmove）。

**HFT**：确保订单 struct 是 trivially copyable（无析构、无虚函数），让 copy 走 memmove。

**复习：** → [特化决策链](./03-copy-specialization.md)
</details>

### Q2: 手写循环和 std::copy 哪个更快？

```cpp
// 方式 A: 手写循环
for (size_t i = 0; i < n; ++i) dst[i] = src[i];

// 方式 B: std::copy
std::copy(src, src + n, dst);
```
> 为什么 B 可能更快？

<details>
<summary>答案与复习指引</summary>

**B 可能更快**的原因：
1. `std::copy` 对 trivially copyable + 随机访问明确特化为 `memmove`
2. `memmove` 可能被编译器进一步优化为 SIMD（SSE/AVX 一次拷贝 16/32 字节）
3. 手写循环编译器**可能**优化为 memcpy，但不保证（取决于编译器、优化级别、循环复杂度）

**但**：现代编译器（`-O2`/`-O3`）对简单手写循环通常也能优化为 `memcpy`/SIMD。差距不大。

**HFT**：用 `std::copy` 的理由不是"更快"，而是"更可靠地走 memmove" + "更可读" + "泛型"。

**复习：** → [memmove 特化](./03-copy-specialization.md)
</details>

### Q3: 下面的代码有什么问题？

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};
// 想把前 3 个元素移到后面
std::copy(v.begin(), v.begin() + 3, v.begin() + 2);
```
> 源和目标重叠会怎样？

<details>
<summary>答案与复习指引</summary>

**源 `[0, 3)` 和目标 `[2, 5)` 重叠**（索引 2 重叠）。`std::copy` 不保证正确处理重叠区域，行为未定义。

可能的结果：`{1, 2, 1, 2, 1}`（因为拷贝到索引 2 时覆盖了还未拷贝的元素 3）。

**正确做法**：
```cpp
// 前向重叠用 copy_backward（从后往前拷）
std::copy_backward(v.begin(), v.begin() + 3, v.begin() + 5);
// 或用 memmove（处理重叠）
std::memmove(v.data() + 2, v.data(), 3 * sizeof(int));
```

**规则**：
- 目标在源之后（`dst > src`）→ 用 `copy_backward`
- 目标在源之前（`dst < src`）→ 用 `copy`
- 不确定 → 用 `memmove`（始终安全）

**复习：** → [源目标重叠](./03-copy-specialization.md)
</details>

### Q4: back_inserter 和预分配 copy 哪个更高效？

```cpp
std::vector<int> src = {1,2,3,4,5};

// 方式 A: back_inserter
std::vector<int> dstA;
std::copy(src.begin(), src.end(), std::back_inserter(dstA));

// 方式 B: 预分配
std::vector<int> dstB(src.size());
std::copy(src.begin(), src.end(), dstB.begin());
```
> A 和 B 的性能差异在哪里？

<details>
<summary>答案与复习指引</summary>

**方式 A（back_inserter）**：每元素调用 `push_back`，可能多次扩容（reallocate + move 所有元素）。即使 `reserve` 后避免了扩容，每元素 `push_back` 仍有函数调用开销。

**方式 B（预分配）**：一次性分配 + `memmove`（trivially copyable），最优路径。

**最佳实践**：
```cpp
// 方式 C: 区间 insert（最简洁）
std::vector<int> dstC;
dstC.insert(dstC.end(), src.begin(), src.end());
// vector::insert 内部会 reserve + memmove
```

**HFT**：热路径用方式 B 或 C，避免 back_inserter 的逐元素 push_back。

**复习：** → [back_inserter 陷阱](./03-copy-specialization.md)
</details>

## 参考与延伸

- 上一节：[6.2 二分查找算法](02-binary-search-algorithms.md)
- 下一节：[6.4 accumulate/for_each](04-accumulate-foreach.md)
- 源码参考：`bits/stl_algobase.h`（`__copy_dispatch` / `__copy_trivial`）
