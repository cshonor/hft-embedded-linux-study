# 2.1.3 · 对象安全 (Object Safety)

> 所属：**Traits and Trait Bounds · 编译与分发** · [← 05 hub](./05-compilation-dispatch.md)

← [05.2 单态化与内存](./05-2-monomorphization-memory.md) · 下一节 [05.4 选型与 HFT](./05-4-selection-hft.md)

---

**并非所有 trait 都能 `dyn Trait`** — vtable 必须为**固定布局**的方法表。

---

## 常见「阻碍 `dyn`」的情况

| 问题 | 例子 | 为何不行 |
|------|------|----------|
| **泛型方法** | `fn bar<T>(&self, x: T)` | 无法为所有 `T` 列 vtable 条目 |
| **返回 `Self`（Sized）** | `fn new() -> Self` | trait object 上 `Self` 大小未知 |
| **要求 `Self: Sized` 的方法** | `fn clone(&self) -> Self where Self: Sized` | `dyn Trait` 是 **DST**，非 Sized |
| **关联类型无约束到 object-safe 用法** | 视具体定义 | 见 Reference / Clippy `object_safe` |

```rust
// ❌ 通常不能 dyn
trait Bad {
    fn generic<T>(&self, x: T);
    fn make() -> Self;
}

// ✅ 常见 object-safe 形态
trait Good {
    fn handle(&self, msg: &str);
    fn handle_mut(&mut self, msg: &str);
}
```

→ 设计 API 时先定：**要不要 trait object？** → [第 3 章 Designing Interfaces](../Chapter-03-Designing-Interfaces/06-object-safety.md)

→ 下一节：[05.4 选型与 HFT](./05-4-selection-hft.md)
