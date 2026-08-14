# 04.6 · 易错点 · 延伸

← [04 所有权索引](./04-ownership.md) · 上一节：[04-5-refs-and-panic.md](./04-5-refs-and-panic.md)

---

## 易错点速查

| 易错 | 正确理解 |
|------|----------|
| 堆一定 Move、栈一定 Copy | 看 **`Copy` trait**；栈上 struct 也可 Move |
| 赋值 = 复制 | 仅 **`Copy`** 类型；`String` 等默认 **move** |
| Move = 复制整段堆 | Move **只转移所有权**；堆复制要用 **`.clone()`** |
| Copy 和 Clone 是一回事 | **Copy** 隐式按位；**Clone** 显式、可深拷贝堆 |
| 局部变量与 struct 字段 drop 同序 | **相反**：局部逆声明，字段正定义 |
| `&x` 会 drop `x` | **不会**；只有所有者 drop |
| panic 一定泄漏 | **unwind 默认会 drop**；`abort` 除外 |
| `Copy` 与自定义 `Drop` | 有自定义 **`Drop` 则不能 `Copy`** |

---

## 延伸（深入另读）

| 主题 | 去向 |
|------|------|
| `ManuallyDrop`、故意泄漏 | Nomicon · [第 9 章 Unsafe](../Chapter-09-Unsafe-Code/README.md) |
| `Pin` 与位置不可移动 | [第 8 章 Async](../Chapter-08-Async/README.md) |
| `async` 状态机里的 drop / cancel | [08 Async](../Chapter-08-Async/README.md) |
| RAII 惯用法 | ER [Item 11 Drop/RAII](../../01-ER/Chapter-02-Traits/Item-11-drop-raii/README.md) |

---

## 下一节

→ [05 共享引用](./05-shared-references.md)（借用不抢所有权）
