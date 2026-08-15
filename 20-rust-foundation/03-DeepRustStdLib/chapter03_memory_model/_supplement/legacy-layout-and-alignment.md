# 3.1 内存布局与对齐

> 章索引：[第 3 章](./README.md) · 后：[3.2 生命周期与借用检查](./3.2-lifetimes-and-borrow-check.md)

---

## 一句话

读 `std` 源码和做 HFT 结构体优化前，先弄清 **大小、对齐、字段顺序、niche 优化** — 它们决定 `Vec<T>` 元素间距、`#[repr(C)]` FFI 与 false sharing。

---

## 核心概念

| 概念 | 含义 | std / 实盘 |
|------|------|------------|
| **size_of** | 类型占多少字节 | `Vec` 三 word 胖指针（ptr/cap/len） |
| **align_of** | 对齐要求 | `AtomicU64` 常要求 8 字节对齐 |
| **padding** | 为对齐插入的空洞 | 自定义 `struct` 字段重排可减 size |
| **niche** | 用无效指针值编码 `Option` | `Option<&T>` 与 `&T` 同大小 |
| **ZST** | 大小为 0 的类型 | `PhantomData`、部分 marker |

---

## 标准库中的布局

- **`Vec<T>` / `String`**：堆上连续 `T`，栈上 `(ptr, len, cap)`。
- **`Arc` 内部**：控制块 + 数据，引用计数与控制块对齐。
- **原子类型**：布局须满足平台原子指令；见 `core::sync::atomic`。

→ [RFR Ch02 · 02 layout](../../02-RFR/Chapter-02-Types/02-layout.md) · [01 alignment](../../02-RFR/Chapter-02-Types/01-alignment.md) · [Nomicon 02 Data Layout](../../04-Rust-Nomicon/02_Data_Layout/README.md)

---

## HFT 提示

- 热路径结构体考虑 **`#[repr(C)]`**（FFI）或 **`#[repr(align(64))]`**（缓存行对齐，慎用 padding）。
- 读 IR 对照布局 → [06 Learn LLVM 17 ch05](../../06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/part02_src_to_machine/chapter05_ir_advanced_type/README.md)

---

## 相关

- [3.8 PhantomData](./3.8-phantomdata.md)（零大小标记）
- [3.10 MaybeUninit](./3.10-maybeuninit.md)（未初始化字节块）
