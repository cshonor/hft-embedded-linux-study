# 附录 A：参考文献

**Bibliography**

## 本附录讲什么

侯捷推荐的 STL 与泛型编程延伸阅读书目。

## 要点

- **《Generic Programming and the STL》**（Matthew Austern）：泛型编程思想与 STL 概念模型。
- **《Effective STL》**（Scott Meyers）：STL 实战要点（本仓库 05 已整理）。
- **《C++ Standard Library》**（Nicolai Josuttis）：标准库全景参考。
- **SGI STL 文档**：最权威的 STL 在线文档（sgi.com/tech/stl）。
- **ANSI/ISO C++ Standard**：最终标准文本。

## 自测题

1. 想深入泛型编程思想该读哪本书？STL 实战要点呢？
2. SGI STL 文档的权威性体现在哪里？

## 代码自测

### Q1: STL 参考资料选择
```cpp
// 读 STL 源码的正确顺序：
// 1. 先用 STL（会 sort/find/vector）
// 2. 读 Effective STL（知道最佳实践）
// 3. 读 STL 源码剖析（理解底层实现）
// 4. 读 C++ standard（权威定义）
```
> 初学者直接读 STL 源码有什么问题？推荐的阅读路径是什么？

<details>
<summary>答案与复习指引</summary>

**直接读源码的问题**：
1. STL 源码大量使用模板元编程、偏特化、宏，初学者难以理解
2. SGI/GNU 源码有大量优化细节，容易迷失在细节中
3. 没有使用经验，无法理解为什么这样设计

**推荐路径**：
1. **先用**：C++Primer/Effective STL 学会正确使用
2. **再读实践**：Effective STL 讲使用中的坑和最佳实践
3. **然后读源码**：STL Source Analysis 带着使用经验理解实现
4. **最后查标准**：需要精确语义时查 C++ Standard

**HFT 额外**：理解 STL 源码才能判断性能特性（如 `vector` 扩容策略、`map` 节点大小），做正确的热路径选型。

**复习：** → [阅读建议](./README.md)
</details>
