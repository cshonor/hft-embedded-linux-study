# 第 7 章 高级对象模型

**On the Cusp of the Object Model**

## 本章讲什么

模板实例化机制、异常处理的底层实现，以及对象模型未来的演进方向。这些是"高级"话题——日常编程少直接接触，但理解它们能深化对 C++ 编译/运行机制的认识。

## 要点

### 模板实例化

模板定义是"配方"，实例化时编译器按具体类型生成代码。实例化时机：隐式（使用点）或显式（`template class vector<int>;`）。重复实例化由链接器合并（COMDAT 折叠）。模板代码膨胀是代价——每种类型一份代码。

### 异常处理实现

现代 C++ 用 **table-based EH**：编译器为每个函数生成异常表（.gcc_except_table），记录哪些区间可被哪些 catch 捕获。抛异常时运行时查表定位 handler + 栈展开。正常路径零开销，异常路径昂贵。

### 模板 + 异常的交互

模板函数的异常规范影响实例化行为。`noexcept` 是类型系统的一部分（C++17 起类型相关），影响函数指针类型。

## HFT 关联

- **模板代码膨胀**：过度模板化增大二进制 + I-cache 压力。HFT 对热路径模板适度，必要时用 `extern template` 显式实例化集中编译。
- **`-fno-exceptions` + 模板**：关异常时模板的 `throw` 被移除，但 STL 部分模板行为变化。

## 自测题

1. 模板实例化的时机是什么？代码膨胀代价是什么？
2. table-based 异常处理如何做到正常路径零开销？
3. 模板代码膨胀对 I-cache 有什么影响？`extern template` 如何缓解？

## 代码自测

### Q1: 模板实例化与代码膨胀
```cpp
template<typename T>
T maxVal(T a, T b) { return a > b ? a : b; }

// 使用
maxVal<int>(1, 2);
maxVal<double>(1.0, 2.0);
maxVal<int>(3, 4);  // 已实例化过
```
> 编译器生成了几份 `maxVal` 的代码？模板实例化对二进制大小有什么影响？

<details>
<summary>答案与复习指引</summary>

生成 **2 份**代码：`maxVal<int>` 和 `maxVal<double>`。第三次调用 `maxVal<int>(3,4)` 复用已实例化的版本。

**代码膨胀**：模板对每种类型生成独立代码。如果对 10 种类型用 `vector<T>`，就生成 10 份 vector 代码。HFT 中大量模板使用会增加二进制大小和 icache 压力。

**对策**：非泛型核心逻辑抽出为非模板函数（`vector<T>` 的核心操作通过 `void*` 实现），模板只做类型安全包装。

**复习：** → [模板实例化](./README.md)
</details>

### Q2: 异常的运行时代价
```cpp
// 方案 A：异常
int divide_ex(int a, int b) {
    if (b == 0) throw std::runtime_error("div by zero");
    return a / b;
}

// 方案 B：返回值
std::optional<int> divide_opt(int a, int b) {
    if (b == 0) return std::nullopt;
    return a / b;
}
```
> 即使不抛异常，方案 A 有运行时代价吗？HFT 通常如何选择？

<details>
<summary>答案与复习指引</summary>

**有代价**。异常机制即使在"不抛"路径上也有开销：
- 编译器生成 **unwind table**（异常处理表），增加二进制大小
- 部分编译器在正常路径上也有分支检查（取决于实现）
- icache 压力增大

但现代编译器（GCC -fno-exceptions）在"零抛出"假设下，正常路径开销可忽略。**HFT 实践**：通常用 `-fno-exceptions` 关闭异常，热路径用返回值/optional/expected 替代。理由是异常的 unwinding 路径不可预测、延迟尖峰。

**复习：** → [异常的运行时代价](./README.md)
</details>
