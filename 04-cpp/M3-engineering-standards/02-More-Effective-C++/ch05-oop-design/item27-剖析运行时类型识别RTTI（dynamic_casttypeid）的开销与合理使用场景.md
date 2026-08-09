# 条款 27：剖析运行时类型识别 RTTI（dynamic_cast/typeid）的开销与合理使用场景

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
Base *bp = getObject();
if (auto *dp = dynamic_cast<Derived *>(bp)) {
    dp->derivedOnly();
}
```

---

## 代码自测

**题目 1：** `dynamic_cast` 和 `typeid` 各有什么开销？何时使用？
```cpp
Base* p = getShape();
Circle* c = dynamic_cast<Circle*>(p);  // 开销？
if (typeid(*p) == typeid(Circle)) { ... }  // 开销？
```

<details>
<summary>参考答案</summary>

`dynamic_cast`：运行时检查 RTTI 信息，向下转型时遍历类层次树——开销 O(深度)。通常 50-100ns。`typeid`：返回 `type_info` 引用，比较两个 `type_info` 是否相等——开销较小。两者都需要 RTTI 信息（编译器默认开启，`-fno-rtti` 可关闭）。合理使用场景：1) `dynamic_cast` 在无法用虚函数替代时；2) `typeid` 用于调试/日志。生产代码中应优先用虚函数消除类型判断。

</details>
