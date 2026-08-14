# 2.3.2 · Coverage 规则与 Blanket Impl

> 所属：**Traits and Trait Bounds · 相干性** · [← 07 hub](./07-coherence-orphan-rule.md)

← [07.1 孤儿规则](./07-1-orphan-rule.md) · 下一节 [07.3 NewType 实践](./07-3-newtype-practice.md)

---

## 一、覆盖规则 Coverage（孤儿规则例外）

满足条件时，允许给**外部类型**实现**外部 Trait**，核心是**本地类型出现在泛型参数等「本地元素」位置**。

### 合法示例

```rust
struct MyData;

// From 是外部 trait，Vec<u8> 是外部类型，但泛型参数 MyData 是本地类型
impl From<MyData> for Vec<u8> {
    fn from(_: MyData) -> Self {
        vec![1, 2, 3]
    }
}
```

拆解：

| 成分 | 来源 |
|------|------|
| Trait `From` | 外部（标准库） |
| 目标 Type `Vec<u8>` | 外部 |
| 泛型参数 `MyData` | **本地** → 满足孤儿规则 |

---

## 二、Blanket Impl（泛型全覆盖实现）

```rust
trait MyLocalTrait {}
impl<T: std::fmt::Debug> MyLocalTrait for T {}
```

1. **合法前提**：`MyLocalTrait` 是你本地定义的 trait。  
2. **潜在隐患**：过度宽泛的 blanket 会**抢占**未来其它 impl，触发 **coherence error**。

举例：后续第三方为 `Vec<u8>` 实现 `MyLocalTrait`，但你的 blanket 已覆盖所有 `T: Debug`（包含 `Vec<u8>`）→ 与「至多一个 impl」冲突。

**工程建议**：blanket 尽量加**紧**的 bound，或配合 **sealed trait** 限制实现范围 → [Ch03 §12](../Chapter-03-Designing-Interfaces/12-trait-implementations.md)。

→ 下一节：[07.3 NewType 与工程实践](./07-3-newtype-practice.md)
