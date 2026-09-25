# 侯捷序

**Foreword (Hou Jie)**

## 要点

侯捷（本书译者）在序中强调：C++ 的面向对象不是"语法糖"，它有真实的内存与运行时代价。很多 C++ 程序员"会用但不懂"，写出看似面向对象实则低效的代码。本书的价值在于把"看不见的对象模型"摊开来——让你知道每个 `virtual`、每次继承、每个构造在底层发生了什么。

侯捷建议：读完本书后用编译器工具（`sizeof`、`offsetof`、`-fdump-class-hierarchy`）实际验证对象布局，把抽象知识落到具体证据上。

## 自测题

1. 侯捷认为 C++ 程序员最普遍的问题是什么？
2. 如何用编译器工具验证对象布局？

<details>
<summary>参考答案</summary>

1. 最普遍的问题是“会使用语言特性，却不了解其底层对象模型和成本”。
   这容易让人把继承、virtual 和构造等机制视为无代价语法，进而写出布局或运行时行为不符合预期的代码。
2. 可先用 `sizeof`、`alignof` 检查大小和对齐；仅对适用的 standard-layout 类型用 `offsetof` 检查成员偏移。
   再用 GCC 的 class-layout dump 或 Clang 的 `-Xclang -fdump-record-layouts`，配合反汇编观察调用；结果应注明典型 Itanium C++ ABI / 对应编译器版本，不当作标准保证。

</details>
