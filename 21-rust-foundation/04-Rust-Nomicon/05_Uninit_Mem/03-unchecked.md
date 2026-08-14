# 3 · 非受检的未初始化内存 (Unchecked)

← [本章目录](./README.md) · 上一节：[02-drop-flags.md](./02-drop-flags.md) · 下一节：[04-vec-prelude.md](./04-vec-prelude.md)

---

Safe Rust **不允许**数组部分初始化 — 对逐元素构建的底层结构过于死板，须借助 Unsafe。

## `MaybeUninit<T>`

标准工具：向编译器标明「可能未初始化」。

- Drop `MaybeUninit` **不会**清理内部（因可能无合法值）
- 覆盖时**不会**误 Drop 旧值
- 确认全部合法后，`assume_init` / `assume_init_read` 或 transmute 转为普通类型

→ 源码：[src/maybe_uninit.rs](./src/maybe_uninit.rs)

## `ptr` 模块

| API | 作用 |
|-----|------|
| `ptr::write` | 盲写目标地址，**绕过**旧值 Drop |
| `ptr::copy` | 类似 C `memmove`（可重叠） |
| `ptr::copy_nonoverlapping` | 类似 C `memcpy` |

滥用极易 UB → 源码：[src/ptr_ops.rs](./src/ptr_ops.rs)

## 废弃的 `mem::uninitialized`

旧代码可能见到；与语言机制**无法修复的冲突**，新代码**永远**用 `MaybeUninit` 替代（见 [00-overview.md](./00-overview.md) 与源码注释）。
