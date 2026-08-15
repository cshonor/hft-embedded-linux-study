# Item 6 · 核心知识点

← [Item 6 目录](./README.md)

### Newtype 模式

- 用**单字段元组结构体**包裹已有类型 → 在类型系统里得到**新的独立类型**，值域相同、**类型不同**。

```rust
pub struct UserId(u64);
pub struct OrderId(u64); // 与 UserId 不能混用
```

### 孤儿规则（Orphan Rule）

- 只有 **trait 或类型至少一方定义在当前 crate**，才能 `impl Trait for Type`。
- 不能为外部库的 `StdRng` 直接 `impl Display`（类型与 trait 都在外部）。

### 类型别名 vs Newtype

| | `type Alias = T` | `struct New(T)` |
|--|------------------|-----------------|
| 编译器视角 | **仍是 `T`** | **新类型** |
| 混用检查 | ❌ 不防呆 | ✅ `mismatched types` |
| 自定义 trait | 不能为新类型 impl | ✅ 本地类型可 impl |

---
