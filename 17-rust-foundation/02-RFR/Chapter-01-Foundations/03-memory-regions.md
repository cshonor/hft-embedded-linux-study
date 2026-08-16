# 1.3 Memory Regions（内存区域）

> 所属：**Talking About Memory** · [← 章索引](./README.md)

Rust 学内存时有 **两种互补分法** — 对应 [02 变量深入](./02-variables-in-depth.md) 里的**高层 / 底层**双模型，也对应 **Safe Rust 规则 vs OS/LLVM 物理布局**：

| | [03.1 Rust 模型](./03-1-rust-memory-model.md) | [03.2 OS / LLVM 布局](./03-2-os-memory-layout.md) |
|---|------------------------------------------------|-----------------------------------------------------|
| **视角** | Safe Rust、所有权、借用 | 操作系统、链接器、虚拟地址、IR |
| **分几类** | **栈 / 堆 / 静态**（三分类） | **Text / Data / BSS / Heap / Stack**（+ mmap 等） |
| **何时用** | 学 Book、写业务、理解 `Box` / 作用域 | 读 IR、FFI、`static` 进哪段、HFT 调优 |
| **和 02 的对应** | **高层 · 数据流** | **底层 · 内存槽 + 地址** |

**阅读顺序**：先 **03.1**（三分类 + 所有权背景）→ 需要时再 **03.2**（五分区 + 现代 VA）。

---

## 两模型如何映射

| Rust 三分类（03.1） | OS 五分区（03.2） |
|---------------------|-------------------|
| 栈 | **Stack** |
| 堆 | **Heap**（`brk` / `mmap`） |
| 静态 | **Data + BSS**（+ 只读 **`.rodata`**） |

代码段 **Text** 不在 Rust 三分类里，但每个可执行程序都有 — 见 [03.2](./03-2-os-memory-layout.md)。

Rust **不改变** OS 布局；语言层只规定**如何安全地使用**栈、堆和静态存储。

---

## 子节导航

### [03.1 · Rust 内存模型（Safe Rust 三分类）](./03-1-rust-memory-model.md)

- 栈 / 堆 / 静态各存什么
- `Box` / `String`：句柄在栈、payload 在堆
- 与所有权、借用、`'static` 的关系

### [03.2 · OS / LLVM 内存布局](./03-2-os-memory-layout.md)

- 五分区逐段（Text / Data / BSS / Heap / Stack）
- 经典简化图 vs **现代 Linux/Windows**（栈堆独立、不对撞）
- MMIO、mmap 文件、NVRAM 等非常规映射

---

## 一句话选型

> **写 safe Rust → 03.1；看机器真实长什么样 → 03.2。**

---

## 延伸阅读

- 变量双模型 → [02 变量深入](./02-variables-in-depth.md)
- 布局 / 对齐 → [第 2 章 · Layout](../Chapter-02-Types/02-layout.md)
- IR：`alloca`（栈）vs heap → [06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17 ch04](../../06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/part02_src_to_machine/chapter04_ir_basic/README.md)
