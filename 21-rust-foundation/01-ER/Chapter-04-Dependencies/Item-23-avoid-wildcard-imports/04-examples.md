# Item 23 · 案例与代码

← [Item 23 目录](./README.md)

### 本地名称掩蔽（通常无害）

```rust
use bytes::*;

struct Bytes(Vec<u8>); // 本地 Bytes 优先，不与 bytes::Bytes 冲突
```

普通类型冲突时，**本地定义优先**于 glob 导入。

### Trait 方法冲突（编译失败）

- 依赖 Minor 新增 trait `BytesLeft`，为 `&[u8]` 提供 `remaining()`
- 你已 glob 导入 `bytes::Buf`，也有 `remaining()`
- `v.remaining()` → **`E0034: multiple applicable items in scope`**

修复：显式导入 + **完全限定语法**：

```rust
BytesLeft::remaining(&v)
// 或 Buf::remaining(&v)
```

---
