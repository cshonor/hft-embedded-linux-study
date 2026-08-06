# 7.1 模板实例化

> 第 7 章 高级对象模型 · 上一节：[本章导读](README.md) · 下一节：[7.2 异常处理实现](02-exception-impl.md)

## 这节讲什么

模板定义是"配方"，实例化时编译器按具体类型生成代码。代价是代码膨胀——每种类型一份代码。

---

## 实例化机制

```cpp
template<class T> class Vector { ... };
Vector<int> vi;    // 编译器生成 Vector<int> 的代码
Vector<double> vd; // 编译器生成 Vector<double> 的代码
// 每种类型一份独立代码 → 二进制膨胀
```

- **隐式实例化**：使用点自动实例化
- **显式实例化**：`template class Vector<int>;` 在某处集中实例化
- **extern template**：`extern template class Vector<int>;` 声明已在别处实例化，抑制本翻译单元的实例化

重复实例化由链接器合并（COMDAT 折叠）。

---

## 新手要点

- **C 没有模板**：C 用 `void*` + 宏模拟泛型（如 `qsort`）。C++ 的模板在编译期生成类型安全的代码——但代价是代码膨胀。
- **extern template**：大型项目用 `extern template` 集中实例化，减少重复编译 + 代码膨胀。

---

## HFT 关联

- **模板代码膨胀**：过度模板化增大二进制 + I-cache 压力。HFT 对热路径模板适度，必要时用 `extern template` 显式实例化集中编译。

---

## 自测题

1. 模板实例化的时机是什么？
2. 代码膨胀代价是什么？
3. `extern template` 如何缓解代码膨胀？

---

## 参考与延伸

- 下一节：[7.2 异常处理实现](02-exception-impl.md)
- 回到：[第 7 章 高级对象模型](README.md)
