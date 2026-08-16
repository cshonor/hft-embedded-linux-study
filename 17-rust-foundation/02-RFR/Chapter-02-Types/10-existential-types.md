# 3. Existential Types（存在类型）

> 所属：**Existential Types** · [← 章索引](./README.md)

← [09 标记 Trait](./09-marker-traits.md) · 下一节 → [11 小结](./11-summary.md)

---

**`impl Trait`** = 存在类型（∃T）：编译器知具体 `T`，对外隐藏名字，**静态分发、零运行时开销**。

```text
返回位  →  藏长迭代器 / async Future
参数位  →  匿名 <T: Trait>
异构集合 →  用 dyn，不用 impl
```

前置 → [09](./09-marker-traits.md) · [05](./05-compilation-dispatch.md) · [04.3 dyn](./04-3-dyn-vtable.md)

---

## 子节导航

| § | 主题 | 阅读 |
|---|------|------|
| **10.1** | ∃ 逻辑 · 返回/参数位置 | [10-1-logic-positions.md](./10-1-logic-positions.md) |
| **10.2** | `impl Trait` vs `dyn Trait` | [10-2-impl-vs-dyn.md](./10-2-impl-vs-dyn.md) |
| **10.3** | 限制 · 选型 · 易混 | [10-3-limits-selection.md](./10-3-limits-selection.md) |
| — | 速记 · 自测 |

**建议阅读顺序**：`10.1` → `10.2` → `10.3`

---

## 一句话

> **`impl Trait` = ∃T 静态隐藏；要异构集合用 `dyn`；双 `impl` 默认可不同类型。**
