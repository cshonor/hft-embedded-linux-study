# Item 23 · 核心知识点

← [Item 23 目录](./README.md)

### 通配符导入（Wildcard / Glob）

```rust
use somecrate::module::*;
```

- 将目标模块**全部公开符号**一次性拉入当前作用域。

### 命名空间污染

- 外部 crate 演进时，未知符号静默进入本地命名空间 → **冲突**、**歧义**、难维护。

---
