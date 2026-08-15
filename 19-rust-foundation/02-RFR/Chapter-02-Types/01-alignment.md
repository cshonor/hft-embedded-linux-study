# 1.1 Alignment（对齐）

> 所属：**Types in Memory** · [← 章索引](./README.md)

类型的主要作用之一：告诉编译器如何**合法、高效**地解释某段内存里的比特。对齐是第一步。

内存分区背景 → [第 1 章 · 03.1 栈/堆/静态](../Chapter-01-Foundations/03-1-rust-memory-model.md) · [03.2 OS 布局](../Chapter-01-Foundations/03-2-os-memory-layout.md)

---

## 核心总结（x86_64）

### 1. 先分清两个概念

| 概念 | 含义 |
|------|------|
| **内存地址** | 每个地址固定对应 **1 字节（8 bit）** 的存储单元 — 与 CPU 是 32 位还是 64 位**无关** |
| **64 位 CPU** | **地址总线** 64 位 → 寻址范围大；数据访问上 x86_64 常以 **8 字节（64 bit）** 为自然 word 粒度 |

不要混成「64 位 = 一个地址存 64 位」— **地址粒度始终是 1 字节**。

---

### 2. 对齐的定义

数据的**起始地址**必须是「自身对齐要求（alignment）」的整数倍。

- Rust 默认：**类型大小 ≤ 对齐要求**，且 alignment 为 2 的幂
- 例：`i64` 占 **8 字节** → 起始地址必须是 **8 的倍数**（如 `0x1000` ✅，`0x1005` ❌）
- 例：`i32` → 4 的倍数；`i8` → 1（任意地址）

```rust
use std::mem::{align_of, size_of};

assert_eq!(size_of::<i64>(), 8);
assert_eq!(align_of::<i64>(), 8);
assert_eq!(align_of::<i32>(), 4);
assert_eq!(align_of::<i8>(), 1);
```

---

### 3. 为什么要对齐

让数据**完整落在 CPU 单次自然读取的宽度内**， ideally **读一次** 取完。

| | 对齐 | 未对齐 (misaligned) |
|---|------|---------------------|
| **x86_64** | 一次 load 取完 | 可能跨两个 8 字节块 → **两次读 + 拼接**，延迟更高 |
| **其它架构** | 正常 | 可能 **trap / UB**（ARM 等更严格） |

**目的**：用可预测的地址规则，换 **load/store 效率** — HFT / 热路径里每一拍都相关。

---

### 4. 生效范围

对齐规则**全局生效**，不由「在栈还是堆」决定：

| 区域 | 谁保证对齐 |
|------|------------|
| **栈**局部变量 | 编译器按类型 layout 分配槽位 |
| **堆**（`Box` / `Vec` / 分配器） | 分配器返回 **满足 `Layout::align()`** 的指针 |
| **静态 / Data / BSS** | 链接器 + 编译器 |
| **只读 `.rodata`** | 同上 |

→ 分区详见 [03.2 OS 布局](../Chapter-01-Foundations/03-2-os-memory-layout.md)

---

### 5. 填充 (padding) 与「碎片」

编译器在 `struct` 字段之间插入 **padding**，使每个字段起始地址满足各自 alignment：

```rust
// 示意：常见 layout（具体以 size_of 为准）
struct Example {
    a: u8,   // 1 字节 @ offset 0
    // 3 字节 padding
    b: u32,  // 4 字节 @ offset 4（须 4 对齐）
}
```

| 问题 | 结论 |
|------|------|
| **空间开销** | 少量 padding — **用极小空间换访问性能** |
| **是不是 malloc 式碎片？** | **不是** — padding 跟类型/变量绑定，随变量/分配块一起释放，不会像堆 free list 那样产生传统外部碎片 |
| **谁管** | 编译器 / 分配器 **自动**完成 |

字段重排、padding 细节 → [02 布局](./02-layout.md)（`repr(Rust)` / `repr(C)` / `packed`）

---

### 6. 使用层面：何时要管

| 场景 | 做法 |
|------|------|
| **普通 safe Rust** | 编译器按类型默认对齐，**无需手动** |
| **FFI / 网络协议 / 硬件寄存器** | 显式 `#[repr(C)]` / `packed` / 手动 layout — 见 [02 布局](./02-layout.md) |
| **低延迟 / 缓存行优化** | 手动 **`#[repr(align(N))]`**（如 64 字节 cache line）、避免 false sharing → [第 10 章 并发](../Chapter-10-Concurrency-and-Parallelism/README.md) |

```rust
#[repr(align(64))] // 整类型按 64 字节对齐（缓存行级优化时才需要）
struct CachePadded {
    counter: u64,
}
```

---

## 自然对齐（规则摘要）

- 硬件常以固定宽度（x86_64 上 **8 字节 word**）为粒度访问。
- 类型的 **alignment** 通常为 2 的幂；**不必**处处等于 `size_of`（如 `[u8; 3]` 大小 3、对齐 1）。
- **非对齐访问 (misaligned access)**：x86_64 多数情况能跑但更慢；在部分架构上为 **UB**。

## 复合类型

- `struct` 的 alignment = 各字段 alignment 的**最大值**。
- 编译器插入 **padding** 满足每个字段；**默认 `repr(Rust)` 可能重排字段** 以减小 size。

```rust
use std::mem::{align_of, size_of};

struct S {
    x: u8,
    y: u64,
}
// size 常为 16（1 + 7 padding + 8），align 为 8
```

---

## 实践清单

| 任务 | 工具 / 动作 |
|------|-------------|
| 查 size / align | `std::mem::size_of` / `align_of` |
| 堆分配 layout | `std::alloc::Layout` |
| FFI 布局 | `#[repr(C)]` + 与 C 头文件对照 |
| 读 IR 里的 align | [06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17 ch05](../../06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/part02_src_to_machine/chapter05_ir_advanced_type/README.md) |
| unsafe 有效性 | 引用须 **对齐** → [第 9 章 validity](../Chapter-09-Unsafe-Code/06-validity.md) |

---

## 易错点

| 易错 | 纠正 |
|------|------|
| 「64 位 CPU = 一个地址 8 字节」 | **1 地址 = 1 字节**；64 位指地址线 / 常用 load 宽度 |
| 「只有堆要对齐」 | **栈 / 堆 / 静态** 全局规则 |
| 「padding = 内存碎片」 | padding 是 **类型 layout 的一部分**，随对象生命周期结束 |
| 「对齐靠程序员逐变量算」 | 默认 **编译器自动**；手动只在 FFI / 性能调优 |

---

## 对照阅读

- 布局 / `repr` → [02 Layout](./02-layout.md)
- Book → [19.3 高级类型 · 内存布局](../../00-Book/19-advanced-features/19.3-高级类型.md)
- 内存槽 → [第 1 章 · 02 变量深入](../Chapter-01-Foundations/02-variables-in-depth.md)
