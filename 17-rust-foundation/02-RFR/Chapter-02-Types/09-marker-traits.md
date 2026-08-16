# 2.5 Marker Traits（标记 Trait）

> 所属：**Traits and Trait Bounds** · [← 章索引](./README.md)

← [08 Trait 限定](./08-trait-bounds.md) · 下一节 → [10 存在类型](./10-existential-types.md)

---

标记 Trait：**无方法、仅语义契约** — `Sized` / `Copy` / `Send` / `Sync` / `Unpin`，编译期零开销。

```text
布局/所有权  →  Sized · Copy · ?Sized
并发        →  Send（移线程）· Sync（共享 &T）
异步        →  Unpin（可移动 vs Pin）
```

前置 → [08.1](./08-1-syntax-static-dynamic.md) · [04.1 DST](./04-1-dst-basics.md)

---

## 子节导航

| § | 主题 | 阅读 |
|---|------|------|
| **09.1** | 标记 vs 普通 Trait | [09-1-marker-basics.md](./09-1-marker-basics.md) |
| **09.2** | `Sized` · `Copy` | [09-2-sized-copy.md](./09-2-sized-copy.md) |
| **09.3** | `Send` · `Sync` · `Unpin` | [09-3-send-sync-unpin.md](./09-3-send-sync-unpin.md) |
| **09.4** | 自动推导 · `unsafe impl` | [09-4-unsafe-impl.md](./09-4-unsafe-impl.md) |
| — | 速记 · 自测 |

**建议阅读顺序**：`09.1` → `09.2` → `09.3` → `09.4`

---

## 一句话

> **`Sized` 默认要、`?Sized` 给 DST；`Send`/`Sync` 手动 impl 须 `unsafe`。**
