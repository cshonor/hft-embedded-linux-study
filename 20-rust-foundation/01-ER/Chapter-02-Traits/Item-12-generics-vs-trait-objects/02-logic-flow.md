# Item 12 · 逻辑脉络

← [Item 12 目录](./README.md)

```text
泛型：编译期特化 → 快、可组合多 bound → 代码体积↑
dyn：  运行期 vtable → 异构集合、省代码 → 间接调用开销
```

### 多重 bound vs 单一 vtable

- 泛型：`T: Debug + Draw` 在**编译期**同时要求两者，可写 `where` 解锁方法。
- `dyn`：一个 vtable 对应**一个** trait；多重 trait 组合在运行期会复杂（supertrait 融合 vtable，见下）。

### Supertrait ≠ OOP 继承

```rust
trait Shape: Draw { fn area(&self) -> f64; }
```

- 表示 **also-implements**（实现 `Shape` 必须也实现 `Draw`），不是「子类 is-a 父类」。
- `dyn Shape` 的 vtable 含 `Draw` 方法，但 **Upcasting** 历史上受限（见 §5、§6）。

---
