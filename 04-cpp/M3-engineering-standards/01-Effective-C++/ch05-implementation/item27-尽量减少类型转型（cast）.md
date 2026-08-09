# 条款 27：尽量减少类型转型（cast）

## 本节讲什么

C 风格强转危险；优先 `static_cast` / `const_cast`，少用 `dynamic_cast` / `reinterpret_cast`；转型会破坏类型安全，重构设计规避转型。

## 示例

```cpp
void f(Base *bp) {
    if (auto *d = dynamic_cast<Derived *>(bp)) { /* ... */ }
    // 避免 C 风格 (Derived*)bp
}
```

---

## 代码自测

**题目 1：** 下面代码的 `static_cast` 有什么隐患？
```cpp
class Base { int bx; };
class Derived : public Base { int dx; };
Base* pb = new Derived;
Derived* pd = static_cast<Derived*>(pb);
pd->dx = 42;  // 安全吗？
```

<details>
<summary>参考答案</summary>

`static_cast` 不做运行时类型检查。如果 `pb` 实际指向的不是 `Derived` 对象（例如 `new Base`），`pd->dx = 42` 会写入错误内存——未定义行为。安全做法：用 `dynamic_cast` 并检查结果。
```cpp
Derived* pd = dynamic_cast<Derived*>(pb);
if (pd) pd->dx = 42;
```
但 `dynamic_cast` 有运行时开销，最好的设计是避免向下转型。

</details>
