# 1.4.2 · 宽指针（Fat / Wide Pointer）

> 所属：**Types in Memory · DST 与宽指针** · [← 04 hub](./04-dst-wide-pointers.md)

← [04.1 DST 基础](./04-1-dst-basics.md) · 下一节 [04.3 dyn 与 vtable](./04-3-dyn-vtable.md)

---

指向 **DST** 的引用 / 裸指针 / `Box`，在 **64 位**上常为 **16 字节** = **两个 `usize`**：

```text
瘦指针 (&T, T: Sized)     [  data ptr  ]           8 B

宽指针 (&[T], &str)        [  data ptr  |  len     ]  16 B
宽指针 (&dyn Trait)        [  data ptr  |  vtable  ]  16 B
```

| 宽指针 | 第一字（data pointer） | 第二字（metadata） |
|--------|------------------------|---------------------|
| **`&[T]` / `&str`** | 首元素 / 字符串起始地址 | **长度**（元素数或字节数） |
| **`&dyn Trait`** | 对象实例地址 | **vtable 指针** |

**要点**：`&[T]`、`&dyn Trait` **本身**是 **Sized**（固定 16B）— 不确定大小的是**它们指向的目标类型**。

---

## 实测（x86_64）

```rust
use std::mem::size_of;

assert_eq!(size_of::<&u32>(), 8);              // 瘦指针
assert_eq!(size_of::<&[u32]>(), 16);           // 宽指针
assert_eq!(size_of::<&str>(), 16);
assert_eq!(size_of::<&dyn std::fmt::Debug>(), 16);
```

→ [`layout-demo`](./layout-demo/) 末尾有 DST 打印

---

## 64 位字节布局（概念图）

**`&[u32]` 指向 3 个元素：**

```text
offset 0..7:   data ptr  → 堆/栈上第一个 u32
offset 8..15:  len = 3
```

**`&dyn Debug`（trait object）：**

```text
offset 0..7:   data ptr  → 具体 struct 实例
offset 8..15:  vtable ptr → 该类型的虚函数表
```

→ 下一节：[04.3 `dyn Trait` 与 vtable](./04-3-dyn-vtable.md)
