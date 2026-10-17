# 附录 C：STLPort 移植

**STLPort Porting**

## 要点

STLPort 是基于 SGI STL 的跨平台开源 STL 实现，曾在 MSVC/Borland 等非标准库环境下流行。本附录讲移植要点：

- **配置宏**：`_STLP_*` 系列宏控制线程模型、异常、调试模式。
- **线程安全**：`_STLP_THREADS` 开启多线程支持（配置器加锁）。
- **调试模式**：`_STLP_DEBUG` 检测迭代器失效/越界（类似 MSVC 的 `_HAS_ITERATOR_DEBUGGING`）。

## 现代意义

C++11 后主流编译器都自带标准库（libstdc++/libc++/MS-STL），STLPort 已基本退出历史舞台。本附录的价值在于理解"STL 实现的可移植性考量"——配置器线程模型、调试模式等概念在现代标准库中仍以不同形式存在。

## 自测题

1. STLPort 解决了什么历史问题？现代还需要它吗？
2. 配置器的线程安全由什么宏控制？与现代标准库的什么机制对应？

## 代码自测

### Q1: STL 移植性考量
```cpp
// 跨平台 STL 注意事项
std::vector<int> v;
v.resize(100);
// v.data() — C++11 起保证连续，返回 T*
// &v[0] — C++03 前的标准写法

// 线程安全
// C++11 保证：const 成员函数线程安全
// 但非 const 操作不保证
```
> 跨平台使用 STL 需要注意哪些差异？线程安全保证是什么？

<details>
<summary>答案与复习指引</summary>

**跨平台差异**：
1. **实现差异**：GCC 2x 扩容 vs MSVC 1.5x；SSO 阈值不同；`sizeof(string)` 不同
2. **ABI 不兼容**：不同编译器的 STL 类型不能跨 DLL/SO 传递（GCC 的 `std::string` 和 MSVC 的内存布局不同）
3. **标准版本**：C++17/20 新特性（`string_view`、`span`）在旧编译器不可用

**线程安全保证**：
- **const 操作**：多个线程同时调 const 成员函数（如 `size()`、`find()`、`operator[] const`）是安全的
- **非 const 操作**：一个线程写，其他线程读写 → **不安全**，需外部同步
- **不同容器**：不同容器对象可以安全地被不同线程操作

**HFT**：假设 STL 容器非线程安全，所有共享数据用 mutex/atomic 保护。

**复习：** → [STL 移植性](./README.md)
</details>
