# Item 50：熟悉 STL 参考资源

> 第 7 章 使用 STL 编程 · Item 50 · 上一节：[Item 49 解读错误信息](item49-read-error-messages.md)

## 为什么要学这个（先建立直觉）

在 C 里，参考资源相对集中——K&R、man 手册、POSIX 文档。C++ STL 更庞大，需要知道去哪里查最新的标准更新。

```c
/* C: 查函数用 man */
// $ man printf
// $ man malloc
// man 手册覆盖大部分 C 标准库
```

```cpp
// C++: STL 太大，man 手册不全
// 需要专门的在线参考和书籍
// cppreference.com 是最权威的在线参考
```

**直觉**：STL 在不断演进（C++11/14/17/20/23），需要知道去哪里查最新信息、如何读标准库源码。

## 这节讲什么

### 在线参考资源

| 资源 | 网址 | 特点 |
|------|------|------|
| **cppreference** | en.cppreference.com | 最权威，覆盖 C++11-23，含示例 |
| **C++ Reference (cplusplus)** | cplusplus.com | 示例丰富，但更新较慢 |
| **ISO C++ 标准** | iso.org | 官方标准文档（付费） |
| **GCC libstdc++ 文档** | gcc.gnu.org | 查看具体实现 |
| **LLVM libc++ 文档** | libcxx.llvm.org | Clang 的标准库实现 |
| **Compiler Explorer** | godbolt.org | 在线编译+汇编，看内联效果 |

### cppreference 使用技巧

```cpp
// 查 std::sort：
// 1. 访问 en.cppreference.com
// 2. 搜索 "sort"
// 3. 查看：
//    - 函数签名（所有重载）
//    - 模板参数要求
//    - 复杂度保证
//    - 示例代码
//    - 参见（相关的 lower_bound, nth_element 等）
//    - C++ 版本标注（C++11/17/20 新增的变体）
```

### 读标准库源码

```cpp
// GCC libstdc++ 源码位置（Linux）：
// /usr/include/c++/<version>/bits/stl_algo.h  — 算法实现
// /usr/include/c++/<version>/bits/stl_vector.h — vector 实现

// 读源码的价值：
// 1. 理解 sort 到底用什么算法（introsort: 快排+堆排+插入排序）
// 2. 理解 vector 扩容因子（GCC: 2x）
// 3. 理解 unordered_map 的哈希策略
```

### 推荐书籍

| 书 | 作者 | 价值 |
|----|------|------|
| The C++ Standard Library (2nd ed) | Nicolai Josuttis | 最全面的 STL 参考 |
| Effective STL | Scott Meyers | 最佳实践（就是本书） |
| STL Tutorial and Reference Guide | Musser/Derge/Saini | STL 设计原理 |
| C++ Concurrency in Action | Anthony Williams | 并发 + STL 线程支持 |

### 工具

```bash
# Compiler Explorer (godbolt.org)
# 在线编译代码，查看汇编输出
# 验证 lambda 是否内联、vector 是否优化

# clang-format
# 统一代码风格

# AddressSanitizer / Valgrind
# 检测内存错误（越界、use-after-free）
```

## 常见错误（新手踩坑）

### 错误 1：用过时的参考

```cpp
// 某些教程还在用 C++03 的写法
// 例如 ptr_fun/mem_fun（C++17 已删除）
// 或 auto_ptr（C++17 已删除）
// 查 cppreference 看标注的 C++ 版本
```

**策略**：始终查 cppreference，注意 C++ 版本标注。

### 错误 2：不看复杂度保证

```cpp
// 不知道 std::find 是 O(n)，在热路径用
// 不知道 std::lower_bound 需要已排序
// 不看 cppreference 的 Complexity 段落
```

**策略**：用每个 STL 组件前，查 cppreference 确认复杂度和前置条件。

### 错误 3：不看异常保证

```cpp
// 不知道 vector::push_back 在扩容时可能抛 bad_alloc
// 不知道 sort 的比较器抛异常时会发生什么
```

**策略**：查 cppreference 的 "Exceptions" 段落。

## 新手要点（和 C 的区别）

| 方面 | C | C++ |
|------|---|-----|
| 主要参考 | man 手册 | cppreference.com |
| 标准演进 | C89/C99/C11 | C++11/14/17/20/23（变化更大） |
| 源码可读性 | glibc 较易读 | libstdc++ 模板较难读 |
| 在线工具 | 少 | godbolt.org（编译+汇编） |

## HFT 关联

- **cppreference 查复杂度**：热路径用每个 STL 组件前确认 O(?) 复杂度
- **godbolt 验证内联**：写完 lambda 比较器后，在 godbolt 上验证是否内联
- **读 libstdc++ 源码**：理解 vector 扩容策略、unordered_map 哈希函数，做精确性能预估

## 代码自测

### Q1: cppreference 查询

```
你需要在有序 vector 中查找第一个 >= 100 的元素。
应该用哪个 STL 算法？复杂度是什么？
```

<details>
<summary>答案</summary>

用 `std::lower_bound`。

```cpp
std::vector<int> v = {1, 30, 50, 100, 100, 200, 300};  // 必须已排序
auto it = std::lower_bound(v.begin(), v.end(), 100);
// it 指向第一个 >= 100 的元素
```

复杂度：O(log n)（二分查找）。

**查 cppreference**：搜索 "lower_bound"，看 Complexity 段落确认是 O(log n)，看 Requirements 确认需要已排序。
</details>

### Q2: 版本标注

```
cppreference 上某函数标注 "C++17"。
如果你的项目用 C++14，能用这个函数吗？
```

<details>
<summary>答案</summary>

**不能**。标注 "C++17" 表示该函数/特性在 C++17 标准中引入。C++14 编译器不提供。

**解决**：
1. 升级编译标准到 `-std=c++17`
2. 或找 C++14 的替代方案

**例如**：
- `std::optional` — C++17，C++14 用 boost::optional
- `std::string_view` — C++17，C++14 用 `const std::string&`
- `std::not_fn` — C++17，C++14 手写取反 lambda
</details>

### Q3: godbolt 验证

```
你想确认 std::sort + lambda 是否被内联。
应该用什么工具？怎么看？
```

<details>
<summary>答案</summary>

用 **Compiler Explorer (godbolt.org)**：

1. 输入代码：
```cpp
#include <algorithm>
#include <vector>
void sort_vec(std::vector<int>& v) {
    std::sort(v.begin(), v.end(), [](int a, int b) { return a < b; });
}
```

2. 选编译器和优化级别（如 GCC -O2）

3. 看汇编输出：
   - 如果比较逻辑直接内联到 sort 的循环中 → 内联成功
   - 如果有 `call` 指令指向 lambda 的函数 → 没内联

**HFT**：热路径的比较器/谓词必须在 godbolt 上验证内联。
</details>

### Q4: 异常保证

```
vector::push_back 的异常保证是什么？
如果元素拷贝构造抛异常，vector 状态如何？
```

<details>
<summary>答案</summary>

**强异常保证（strong exception guarantee）**：

如果 `push_back` 过程中元素拷贝/移动构造抛异常：
- vector 状态不变（元素和之前一样）
- 已分配的新内存被释放
- 异常向上传播

**条件**：元素类型的移动构造是 `noexcept` 时，vector 用移动而非拷贝，更安全更高效。

```cpp
struct Widget {
    Widget(Widget&&) noexcept { /* ... */ }  // noexcept 移动
    Widget(const Widget&) { /* 可能抛异常 */ }
};
// vector 优先用 noexcept 移动 → 强保证
```

**HFT**：热路径类型定义 `noexcept` 移动构造，让 vector 用移动语义避免拷贝开销。

**查 cppreference**：看 "Exceptions" 段落确认异常保证。
</details>

## 参考与延伸

- 上一节：[Item 49 解读错误信息](item49-read-error-messages.md)
- M0 C++Primer 全书索引
- [M1 Effective Modern C++](../../../M1-modern-cpp/01-Effective-Modern-C++/README.md)
- 在线参考：[cppreference.com](https://en.cppreference.com/)
