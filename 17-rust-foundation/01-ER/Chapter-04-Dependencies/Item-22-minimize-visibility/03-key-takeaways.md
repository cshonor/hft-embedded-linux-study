# Item 22 · 重点结论

← [Item 22 目录](./README.md)

### 最小可见性原则

- 非公开 API 的一部分 → **尽量收窄**可见性。
- 与 *Effective Java*（访问器优于公开字段）、*Effective C++*（接口不暴露数据成员）同宗。

### 内部共享：优先 `pub(crate)`

- 多模块共用、但**不属于对外 API** → `pub(crate)`，不要直接 `pub`。

### 封装换自由度

- 公开字段 = 承诺布局；改 `Vec` → `HashMap` 等优化 → **Major**。
- 私有字段 + 方法 = 实现可换。

---
