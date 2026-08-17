# 附录 B：网站资源

**Web Resources**

## 要点

- **SGI STL**：`sgi.com/tech/stl` — 最经典的 STL 在线文档与源码。
- **cppreference.com** — 当前最权威的 C++ 标准库在线参考（含 C++17/20 更新，推荐日常查询）。
- **cplusplus.com** — 教程式参考（较浅，适合入门）。
- **gcc/libstdc++ 源码**：读 `<bits/stl_*.h>` 理解实现。
- **llvm/libc++ 源码**：对照另一种实现。

## HFT 实践

读标准库源码（libstdc++）是理解 STL 性能细节的最直接途径——cppreference 给契约，源码给实现。HFT 调优时常需翻 `stl_vector.h`/`hashtable` 源码确认内存模型。

## 代码自测

### Q1: STL 学习资源
```cpp
// 推荐的 STL 深入学习资源
// 1. cppreference.com — 最准确的在线参考
// 2. SGI STL 文档 — 经典（虽过时但概念清晰）
// 3. GCC libstdc++ 源码 — 实际实现
// 4. CppCon talks — 实践经验分享
```
> 为什么读 SGI STL 文档仍有价值？它和现代 C++ 有什么差异？

<details>
<summary>答案与复习指引</summary>

**SGI STL 文档的价值**：
1. **设计思想**：六大组件架构、泛型编程理念至今不变
2. **概念清晰**：SGI 文档对 iterator category、traits 等概念的解释比标准更易懂
3. **历史背景**：理解 STL 演化（SGI → HP → 标准 C++）

**与现代 C++ 的差异**：
- SGI 的 `hash_map` → C++11 `unordered_map`
- SGI 的 `rope` → 未进标准
- C++11 起 `iterator_traits` 不再要求迭代器定义 5 个 typedef（可用 `iterator_base` 辅助）
- C++20 引入 `ranges` 和 `concepts`，迭代器分类更精细

**复习：** → [学习资源](./README.md)
</details>
