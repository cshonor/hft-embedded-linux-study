# 1.4 封装的代价

> 第 1 章 · 上一节：[1.3 继承布局](03-inheritance-layout.md) · 下一章：[第 2 章 构造函数语义](../ch02-constructor-semantics/README.md)

## 这节讲什么

C++ 的封装（`private`/`public`）运行时零开销——访问控制是编译期检查。真正的代价来自虚函数、虚基类、多继承。

---

## 核心结论

```
private/public/protected → 编译期检查 → 运行时零开销
虚函数 → vtable 间接 → 运行时代价
虚基类 → 偏移表间接 → 运行时代价
多继承 → 布局膨胀 + this 调整 → 运行时代价
```

C++ 的设计哲学：**你不使用的东西，你不需要付出代价**（zero-overhead principle）。不用虚函数 = 没有虚函数开销；不用异常 = 没有异常开销。

---

## 新手要点（和 C 的区别）

- **C 没有访问控制**：C 的 struct 所有成员都是 public。C++ 的 `private`/`public` 是编译期检查——编译通过后运行时没有区别。
- **零开销原则**：C++ 的核心哲学——高级特性（虚函数、异常、RTTI）不用就不付代价。这是 C++ 比 Java 高效的根本原因（Java 所有方法默认虚函数）。

---

## HFT 关联

- **零开销是 HFT 选 C++ 的原因**：不用虚函数 = 无 vtable 开销；`-fno-exceptions` = 无异常开销；`-fno-rtti` = 无 RTTI 开销。HFT 热路径只付用到的特性的代价。

---

## 自测题

1. C++ 的 `private`/`public` 访问控制有运行时开销吗？
2. C++ 真正的封装代价来自哪里？
3. "零开销原则"是什么意思？它和 Java 有什么不同？
4. 为什么 HFT 选 C++ 而非 Java？

---

## 参考与延伸

- 下一章：[第 2 章 构造函数语义](../ch02-constructor-semantics/README.md)
- 回到：[第 1 章 关于对象](README.md)
