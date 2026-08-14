# Item 21 · 案例与代码

← [Item 21 目录](./README.md)

### 破坏性变更速查（需升 MAJOR）

| 变更 | 为何 break |
|------|------------|
| 向 enum **加变体**（非 `#[non_exhaustive]`） | 外部 `match` 未穷尽 |
| 向 struct **加字段**（可外部构造） | 字面量/构造报错 |
| trait **失去 object safety** | `dyn Trait` 用法失效 |
| 已有 trait **加 blanket impl** | 与手动 impl 冲突 |
| 改 **License** / **MSRV** | 广义 API 契约变化 |
| 移除或改 **默认 features** | 下游行为突变 |

### `deprecated` 过渡示例

```rust
#[deprecated(since = "1.3.0", note = "use new_api instead")]
pub fn old_api() { /* ... */ }

pub fn new_api() { /* ... */ }
```

---
