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
