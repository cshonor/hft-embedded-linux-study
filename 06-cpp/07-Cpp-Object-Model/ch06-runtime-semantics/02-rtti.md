# 6.2 RTTI（运行时类型识别）

> 第 6 章 · 上一节：[6.1 new/delete 链路](01-new-delete-chain.md) · 下一节：[6.3 异常处理开销](03-exception-cost.md)

## 这节讲什么

`dynamic_cast` 和 `typeid` 的运行时代价——查类型信息表 + 字符串比较。RTTI 只对多态类型（有虚函数）有效。

---

## 两种 RTTI

### dynamic_cast

```cpp
Base* b = new Derived;
Derived* d = dynamic_cast<Derived*>(b);  // 向下转型
// 运行时检查 b 的真实类型
// 代价：查 type_info 表 + 字符串比较（类名）
// 失败：指针返回 nullptr，引用抛 bad_cast
```

### typeid

```cpp
typeid(*b).name()  // 返回类名（mangled name）
```

RTTI 靠 vtable 里的 type_info 槽定位——只对**多态类型**（有虚函数）有效。非多态类型的 RTTI 是静态的（编译期确定）。

---

## 新手要点（和 C 的区别）

- **C 没有 RTTI**：C 的类型在编译期就确定了，运行时没有类型信息。C++ 的 RTTI 依赖 vtable——只有有虚函数的类才有运行时类型信息。
- **`dynamic_cast` 有代价**：不是免费的——查表 + 字符串比较。热路径别用。

---

## HFT 关联

- **禁 `dynamic_cast` 热路径**：RTTI 的类型表查找有 cache miss + 字符串比较代价。用 `enum` 标签 + `static_cast` 替代（编译期保证安全）。
- **`-fno-rtti`**：部分 HFT 引擎整体关 RTTI，减小二进制 + 略加速。

---

## 自测题

1. `dynamic_cast` 只对什么类型有效？
2. 它的运行时代价是什么？
3. HFT 热路径为什么禁 `dynamic_cast`？用什么替代？
4. `-fno-rtti` 有什么利弊？

---

## 参考与延伸

- 下一节：[6.3 异常处理开销](03-exception-cost.md)
- 回到：[第 6 章 运行时语义](README.md)
