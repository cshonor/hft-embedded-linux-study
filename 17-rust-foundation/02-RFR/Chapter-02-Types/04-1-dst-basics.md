# 1.4.1 · DST（动态大小类型）

> 所属：**Types in Memory · DST 与宽指针** · [← 04 hub](./04-dst-wide-pointers.md)

← [03 复合类型](./03-complex-types.md) · 下一节 [04.2 宽指针](./04-2-wide-pointers.md)

前置 → [03 复合类型 · Niche / 枚举](./03-complex-types.md) · [02 layout](./02-layout.md)

---

## DST 是什么

**编译期无法确定 `size_of` 的类型** — 因此：

- **不能**按值直接放栈：`let s: str = "hello";` ❌
- **必须**通过指针 / 引用 / `Box` / `Rc` 等**间接持有**

### Rust 里两类典型 DST

| 类别 | 例子 | 编译期未知的是什么 |
|------|------|-------------------|
| **切片** | `[T]`、`str` | **长度**（元素个数 / 字节数） |
| **Trait 对象底层** | `dyn Trait` | **具体实现类型**的大小 + vtable |

- **`str`**：UTF-8 字节序列的 DST；`&str` = 对 `str` 的宽引用
- **`[T]`**：任意长度连续 `T`；`&[T]`、`Vec<T>` 的视图都涉及长度元数据

**Sized vs DST**：`T: Sized` 才能 `let x: T`、按值传参（默认）；DST 只能出现在 **`&` / `&mut` / `Box` / …** 之后。

→ 下一节：[04.2 宽指针](./04-2-wide-pointers.md)
