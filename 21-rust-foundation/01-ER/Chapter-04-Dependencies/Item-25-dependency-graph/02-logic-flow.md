# Item 25 · 逻辑脉络

← [Item 25 目录](./README.md)

```text
Cargo.toml  semver 范围 → 解析 → Cargo.lock 固化
         ↓
二进制：提交 lock → 可复现构建
库：     不提交 lock（下游忽略）→ 本地/CI 可保留 lock 测上游突变
         ↓
多版本：Rust 类型隔离 OK；API 暴露则灾难（Item 24）
FFI crate：ODR → 多版本 C 符号冲突，Cargo 魔法失效
```

---
