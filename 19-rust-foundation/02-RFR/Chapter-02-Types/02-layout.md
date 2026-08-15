# 1.2 Layout（布局）

> 所属：**Types in Memory** · [← 章索引](./README.md)

前置 → [01 对齐](./01-alignment.md)（单字段为何要对齐）· 内存分区 → [03.2 OS 布局](../Chapter-01-Foundations/03-2-os-memory-layout.md)

---

## 布局 vs 对齐

| 概念 | 管什么 |
|------|--------|
| **内存对齐 (alignment)** | 每个字段 / 类型的**起始地址**规则（2 的幂）→ [01 对齐](./01-alignment.md) |
| **内存布局 (layout)** | 结构体**字段排列顺序**、**offset**、**padding**、**总 size** — 由 **`repr`** 控制 |

对齐是「每个字段站哪条地址线合法」；布局是「多个字段在 struct 里怎么排队、中间塞多少空字节」。

---

## 对齐在布局里的作用（摘要）

- 字段起始地址必须是**该字段 alignment 要求**的整数倍（通常为 2 的幂；与 `size_of` 相关但不必相等）。
- **32 位**平台常见自然 word **4 字节**；**64 位（x86_64）** 常见 **8 字节** — 见 [01 · x86_64 总结](./01-alignment.md)。
- 未对齐 → 降速或 UB；布局的工作之一就是插入 **padding** 满足对齐。

---

## 三种关键 `repr`

### 对照总表

| `repr` | 字段顺序 | padding | 体积 | 可预测 | 典型用途 |
|--------|----------|---------|------|--------|----------|
| **`Rust`（默认）** | 编译器**可重排** | 自动，偏省空间 | **较小** | ❌ 跨版本不保证 | 日常 Rust |
| **`C`** | **源码书写顺序** | 按 C 规则插入 | 常**更大**（空隙多） | ✅ FFI / 协议 | C 互操作、固定二进制格式 |
| **`packed`** | 顺序固定 | **尽量无** | **最小** | ✅ 但危险 | 网络包、硬件寄存器映射 |

---

### 1. 默认 `repr(Rust)` — Rust 原生布局

- 编译器**可重排字段**：把小字段塞进对齐产生的空隙里。
- **仍满足**每个字段自身的对齐要求 — 不牺牲 CPU 访问合法性。
- **优点**：struct **更小**，padding 更少。
- **缺点**：字段顺序**不固定**、跨编译器版本**不可依赖** → **不能**用于 FFI / 磁盘协议。

```rust
#[derive(Debug)]
struct DefaultLayout {
    a: u8,
    b: u64,
    c: u8,
}
// 编译器可能重排 a/c，减少 padding；具体以 size_of / offset_of 为准
```

---

### 2. `repr(C)` — C 兼容布局

- **严格按源码书写顺序**排列，**不重排**。
- 字段之间按 **C 语言对齐规则** 自动 **padding**。
- **缺点**：容易产生较大 **内部空隙**（struct 总 `size_of` 变大）— 这是 **layout padding**，不是堆 malloc 碎片。
- **优点**：布局**稳定、可预测** → **FFI**、读写字节流 / 文件 / 与 C `struct` 一一对应。

```rust
#[repr(C)]
struct CLayout {
    a: u8,   // offset 0
    // 7 bytes padding（64 位下对齐 u64）
    b: u64,  // offset 8
    c: u8,   // offset 16
    // 7 bytes padding（struct 整体 align 8）
} // size_of 常为 24
```

与 C 头文件对接、生成/bindgen 时**必须**用 `repr(C)`（或更明确的 `repr(C, u8)` 等）。

---

### 3. `repr(packed)` — 紧凑布局

- **强行取消字段间对齐 padding**（或压到 `repr(packed(N))` 指定上限）。
- **极致压缩**体积，字段可能从**非对齐地址**开始。
- **代价**：访问可能 **misaligned** → 慢；部分 CPU **fault**；Rust 里可能 **UB**。
- **仅用于**：必须逐字节匹配的外部二进制格式；读完后尽快拷贝到对齐的 Rust 类型。

```rust
#[repr(packed)]
struct Packed {
    a: u8,
    b: u32, // 可能从 offset 1 开始 — 未对齐
}
// 读 b 常需 copy 到局部变量或 unsafe 解引用
```

→ unsafe / validity → [第 9 章](../Chapter-09-Unsafe-Code/06-validity.md)

---

## 实测：`Test { a: u8, b: u32, c: u8 }`（x86_64）

可运行 demo → [`layout-demo/`](./layout-demo/)：

```bash
cargo run --manifest-path 02-RFR/Chapter-02-Types/layout-demo/Cargo.toml
```

```rust
#[derive(Debug)]
struct TestRust { a: u8, b: u32, c: u8 }

#[repr(C)]
#[derive(Debug)]
struct TestC { a: u8, b: u32, c: u8 }
```

**本机 x86_64（Windows）实测输出**（stable，`offset_of!` 已稳定）：

```text
--- repr(Rust) default ---
  size_of  = 8
  align_of = 4          ← 取字段 align 最大值（u32=4）；无 u64 时不是 8
  offset a = 4          ← 编译器重排：b 放到 offset 0
  offset b = 0
  offset c = 5

--- repr(C) ---
  size_of  = 12
  align_of = 4
  offset a = 0          ← 严格源码顺序；a 后 3 字节 padding
  offset b = 4
  offset c = 8          ← c 后还有 3 字节 tail padding（凑整 struct align）

--- repr(packed) ---
  size_of  = 6
  align_of = 1
  offset a = 0
  offset b = 1          ← 无 padding，b 从未对齐地址开始
  offset c = 5
```

**读图**：

| | `repr(Rust)` | `repr(C)` |
|---|--------------|-----------|
| **size** | **8**（重排压缩） | **12**（a + 3 pad + b + c + 3 pad） |
| **字段顺序** | **源码 ≠ 内存顺序**（`b` 被挪到最前） | **源码 = 内存顺序** |
| **`offset_of!(Test, b)`** | **0** | **4**（a 占 1 + 3 字节 pad） |

若 struct 含 **`u64` / `i64`**，`align_of` 在 x86_64 上才会到 **8** — 取决于**最大字段对齐**，不是「64 位系统一律 8」。

---

## 同一字段集：三种 repr 对比（代码模板）

```rust
use std::mem::{align_of, offset_of, size_of};

macro_rules! show_layout {
    ($name:ty) => {
        println!(
            "{}: size={}, align={}, a@{}, b@{}, c@{}",
            stringify!($name),
            size_of::<$name>(),
            align_of::<$name>(),
            offset_of!($name, a),
            offset_of!($name, b),
            offset_of!($name, c),
        );
    };
}

#[derive(Debug)]
struct RustLayout { a: u8, b: u32, c: u8 }

#[repr(C)]
struct CLayout { a: u8, b: u32, c: u8 }

#[repr(packed)]
struct PackedLayout { a: u8, b: u32, c: u8 }

fn main() {
    show_layout!(RustLayout);
    show_layout!(CLayout);
    show_layout!(PackedLayout);
}
```

**不要死记上表数字** — 以本机 `cargo run` 为准；换字段或 `#[repr(C, align(8))]` 结果会变。

---

## 如何查看布局（由浅入深）

### 1. 标准库 — 最常用（无第三方）

**stable** 可用：`offset_of!` 自 **Rust 1.77** 起稳定（不必 nightly / 1.79）。

```rust
use std::mem::{align_of, offset_of, size_of};

#[repr(C)] // 固定顺序，便于对照 padding
struct MyStruct {
    a: u8,
    b: u32,
    c: u16,
}

fn main() {
    println!("size_of  = {}", size_of::<MyStruct>());   // x86_64: 12
    println!("align_of = {}", align_of::<MyStruct>());   // 4
    println!("a @ {}", offset_of!(MyStruct, a));        // 0
    println!("b @ {}", offset_of!(MyStruct, b));        // 4  ← a 后 3 字节 pad
    println!("c @ {}", offset_of!(MyStruct, c));        // 8
}
```

**读 offset**：`b@4` 说明 `a`（1 字节）后有 **3 字节 padding**；总长 12 = 8+2 字段 + 2 尾部 pad（对齐到 4 的倍数）。

完整可运行示例 → [`layout-demo/`](./layout-demo/)（含 `repr(Rust)` vs `repr(C)` 对比）。

| API | 作用 |
|-----|------|
| **`size_of::<T>()`** | 类型总字节（含尾部 padding） |
| **`align_of::<T>()`** | 整体 alignment |
| **`offset_of!(T, field)`** | 字段字节偏移 |

---

### 2. 原始字节 — 调试 padding（`unsafe`，仅学习）

把 struct 当字节切片打印，**padding 区可能是栈上垃圾值**，但**位置**仍能从 `offset_of` 推断：

```rust
use std::mem::size_of;

#[repr(C)]
struct MyStruct { a: u8, b: u32, c: u16 }

fn dump_bytes<T>(label: &str, val: &T) {
    let n = size_of::<T>();
    let ptr = val as *const T as *const u8;
    let bytes = unsafe { std::slice::from_raw_parts(ptr, n) };
    println!("{label} ({n} bytes): {:02x?}", bytes);
}

fn main() {
    let s = MyStruct { a: 0x12, b: 0x5678_abcd, c: 0xef90 };
    dump_bytes("MyStruct", &s);
    // 典型 x86_64：字段字节可见；offset 1–3、10–11 为 padding（值随机）
}
```

⚠️ **仅调试**；生产勿靠此解析 struct。Prefer `offset_of!` + 协议文档。

---

### 3. 第三方 / 工具链

| 工具 | 用途 |
|------|------|
| **[`memoffset`](https://docs.rs/memoffset)** crate | 旧 Rust 无内置 `offset_of!` 时的替代 |
| **`rust-layout`** 等 CLI | 终端打印字段树 + padding 区间（需单独 `cargo install`，版本依 crate 而定） |
| **GDB / LLDB / VS Code 调试器** | 查看变量地址与内存窗口 |
| **`readelf` / `objdump` / LLVM IR** | 链接后类型、`*alloca` `align` → [06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17 ch05](../../06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/part02_src_to_machine/chapter05_ir_advanced_type/README.md) |

旧版 offset 示例：

```toml
# Cargo.toml — 仅 Rust < 1.77 时需要
memoffset = "0.9"
```

```rust
use memoffset::offset_of;
println!("{}", offset_of!(MyStruct, b));
```

---

### 4. 固定布局 + 反汇编

依赖布局的代码（FFI、磁盘格式）：

```rust
#[repr(C)]
struct MyStruct { a: u8, b: u32, c: u16 }
```

配合调试器 / IR / `layout-demo` 验证。**不要**依赖默认 `repr(Rust)` 的 offset。

---

### 关键注意事项

1. **`repr(Rust)` 布局不保证稳定** — 编译器可重排；跨版本可能变。依赖布局 → **`repr(C)`** / **`repr(transparent)`** 等。
2. **枚举 niche** — `Option<&T>` 可能与 `&T` 同大 → [03 复合类型](./03-complex-types.md)。
3. **padding 为性能** — 满足 alignment；见 [01 对齐](./01-alignment.md)。

堆分配 layout → `std::alloc::Layout`（size + align 交给分配器）。

---

## 选型一句话

| 需求 | 选 |
|------|-----|
| 普通 Rust 结构体 | 默认 **`repr(Rust)`** |
| 给 C / 固定二进制用 | **`repr(C)`** |
| 逐字节贴协议、能承担 unsafe | **`repr(packed)`** + 极度谨慎 |

---

## 易错点

| 易错 | 纠正 |
|------|------|
| 「布局 = 对齐」 | **对齐**是规则；**布局**是字段顺序 + padding 的具体排法 |
| 「`repr(C)` 更省内存」 | 通常 **更占字节**（不重排、空隙多） |
| 「默认 struct 字段顺序 = 源码顺序」 | **只有 `repr(C)` / `packed`** 保证顺序 |
| 「`packed` 只是体积小」 | 可能 **UB / crash**，不是免费午餐 |
| 「padding = 堆碎片」 | struct **内部**填充；随对象一起释放 |
| 「`offset_of` 要 nightly / 1.79+」 | **stable 1.77+** 内置 `offset_of!` |

---

## 延伸

- 枚举 niche optimization → [03 复合类型](./03-complex-types.md)
- 自定义布局 / unsafe → [第 9 章 Unsafe](../Chapter-09-Unsafe-Code/README.md)
- Book → [19.3 高级类型 · 内存布局](../../00-Book/19-advanced-features/19.3-高级类型.md)
