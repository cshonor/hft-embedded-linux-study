# 2.4 Safety（安全封装）

> 所属：**Types Across Language Boundaries** · [← 章索引](./README.md)

## 生命周期编码不变式

C 文档：「`Device` 不得长于 `Context`」→ **`Device<'a>`** 持 **`&'a Context`**。

## `Send` / `Sync`

勿默认 **`unsafe impl Send/Sync`**。

C 非线程安全 → API 形态限制（**`!Send` 句柄**、仅在单线程 runtime 暴露）。

**`PhantomData` 不决定 `Send`/`Sync`** — 不能单靠它撤销 `Send`；见官方文档。

## 消灭 `*mut c_void`

为不透明句柄定义 **ZST 标记**：

```rust
pub struct Foo(c_void);
// 用 *mut Foo 区分 A/B，防混传
```

→ [第 3 章 · 包装类型](../Chapter-03-Designing-Interfaces/04-wrapper-types.md)

ER → [Item 34](../../01-ER/Chapter-06-Beyond-Standard-Rust/Item-34-ffi-boundaries/README.md)
