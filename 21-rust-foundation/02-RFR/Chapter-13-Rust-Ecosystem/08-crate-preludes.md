# 2.4 Crate Preludes（Crate 预导入）

> 所属：**Patterns in the Wild** · [← 章索引](./README.md)

把常用类型与扩展 trait **`pub use`** 到 **`my_crate::prelude::*`**，降低入门摩擦。

## 注意

扩展 trait 的方法仍须 **trait 在作用域** — Rust 方法解析规则不变。

```rust
use my_crate::prelude::*;
```

→ [第 5 章 · 避免通配符 import](../../01-ER/Chapter-04-Dependencies/Item-23-avoid-wildcard-imports/README.md)（ER 对 `*` 的谨慎与此不同层）
