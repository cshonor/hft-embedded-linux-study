# Item 24 · 逻辑脉络

← [Item 24 目录](./README.md)

```text
dep-lib API 用 rand 0.7 的类型
app 直接用 rand 0.8 构造对象
         ↓
同名 ThreadRng，不同版本 → 类型断层，无法传入 API
         ↓
用户被迫写 wrapper crate 对齐版本（极烦）
         ↓
库作者：pub use rand; → 下游用 dep_lib::rand 拿「对的那版」
```

与 Item 21：公开依赖的类型若依赖升 **Major**，你的 crate 通常也须 **Major**。

---
