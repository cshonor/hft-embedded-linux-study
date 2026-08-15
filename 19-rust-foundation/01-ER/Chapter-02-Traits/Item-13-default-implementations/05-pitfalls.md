# Item 13 · 易错细节

← [Item 13 目录](./README.md)

### 默认 trait 方法 vs 固有方法同名

- trait **新增**带 default 的方法名，若与类型 **inherent impl** 同名：
  - 普通调用 **`obj.method()`** → **固有方法优先**（遮蔽 trait 默认）。
  - 要调 trait 版本：**完全限定语法**  
    `<Concrete as Trait>::method(&obj)`

→ 演进 public trait 时注意命名，避免与常见 inherent 方法冲突。

---
