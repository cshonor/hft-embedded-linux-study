# 2.1 Juggling Raw Pointers（裸指针）

> 所属：**Great Power** · [← 章索引](./README.md)

← [01 unsafe 关键字](./01-unsafe-keyword.md) · 下一节 [03 调用 unsafe 函数](./03-calling-unsafe-functions.md)

前置 → [Ch01 裸指针术语](../Chapter-01-Foundations/01-memory-terminology.md) · [01 unsafe 五类超能力](./01-unsafe-keyword.md)

---

## 语法

| 类型 | 含义 |
|------|------|
| `*const T` | 不可变裸指针（类似 C `const T*`） |
| `*mut T` | 可变裸指针（类似 C `T*`） |

**两点核心**：

1. **不受**生命周期、别名（借用）规则约束；**可以为 null**。
2. 指针是否合法、指向内存是否有效 — **完全由开发者自行保证**。

---

## 一、`*const T` / `*mut T` 核心特性

### 1. 安全边界

裸指针脱离 Rust **所有权、借用检查、生命周期** 系统：

| 操作 | 是否需要 `unsafe` |
|------|-------------------|
| **创建**裸指针（如 `&x as *const T`） | ✅ Safe |
| **解引用**裸指针（读/写 `*ptr`） | ❌ 必须 `unsafe {}` |

> **辨析**：`&T` / 智能指针的 `*` **不需要** unsafe — 只有裸指针手动解引用才要。→ Book [15.2.2 Deref 速记/unsafe 辨析](../../00-Book/15-smart-pointers/15.2.2-Deref解引用与unsafe速记.md)

编译器**不会**拦截：越界、空指针、悬垂指针、别名冲突等 — 违背约束 → **UB**。

```rust
let mut x = 10;
let p: *mut i32 = &mut x;   // ✅ Safe：创建

unsafe {
    *p += 1;                // ✅ 解引用须 unsafe
}
```

### 2. 可空性

原生裸指针**允许为 null**；使用前须手动判空：

```rust
let p: *mut i32 = std::ptr::null_mut();
if !p.is_null() {
    unsafe { *p = 1; }
}
```

### 3. 转换关系

| 转换 | 是否 Safe |
|------|-----------|
| `*mut T` → `*const T` | ✅ 隐式/强转均可（可变→只读） |
| `*const T` → `*mut T` | ❌ 须 `unsafe`（只读→可变，可能违反别名假设） |
| `&T` / `&mut T` → 对应裸指针 | ✅ `as *const T` / `as *mut T` |

```rust
let mut x = 0;
let mut_ptr: *mut i32 = &mut x;
let const_ptr: *const i32 = mut_ptr;  // ✅ *mut → *const
```

### 4. 与引用的本质区别

| | `&T` / `&mut T` | `*const T` / `*mut T` |
|---|-----------------|------------------------|
| 生命周期 | 编译期约束 | **无** |
| 别名规则 | 编译期互斥 | **无** |
| 可 null | ❌ | ✅ |
| 解引用 | Safe（在借用合法时） | **unsafe** |

→ [Ch01 术语：裸指针 vs 借用](../Chapter-01-Foundations/01-memory-terminology.md)

---

## 二、三大典型用途

### 1. 自引用结构 + `Pin`

普通引用的生命周期**无法**表达「结构体内部字段引用自身内存」；须用裸指针持有自引用字段。  
为防止结构体被 **move** 导致指针悬垂，常结合 **`Pin`** 固定内存地址 — 自引用数据结构的标准方案。

→ [第 8 章 Pin](../Chapter-08-Asynchronous-Programming/06-pin-unpin.md)

### 2. 配合 `Arc` 等运行时寿命管理

`Arc` 用引用计数管理堆上数据寿命。裸指针常在**库内部**绕过静态借用检查，实现弱引用、内部可变等逻辑，**对外仍暴露 Safe API**（类似 `RefCell` + `UnsafeCell` 的模式）。

→ [Ch10 `Arc`](../Chapter-10-Concurrency-and-Parallelism/README.md) · [Ch01 内部可变](../Chapter-01-Foundations/07-interior-mutability.md)

### 3. FFI 跨语言边界

C/C++ 大量使用裸指针；Rust 与 C 交互时，内存地址传递的标准类型就是 `*const T` / `*mut T`。

→ [第 11 章 FFI](../Chapter-11-Foreign-Function-Interfaces/11-外部函数接口-FFI-深度解析.md) · Book [19.1](../../00-Book/19-advanced-features/19.1-不安全Rust.md)

---

## 三、`NonNull<T>` — 非空裸指针包装

当指针**保证永远不为 null** 时，优先使用 `NonNull<T>`，而非原生 `*mut T`。

### 1. 编译期语义：永非 null

- 内部封装 `*mut T`；
- `NonNull::new(ptr)` → `Option<NonNull<T>>`：`ptr` 为 null 时返回 `None`，构造路径强制你处理空指针；
- `NonNull::from(&mut T)` / `from_ref` — 引用来源天然非空。

### 2. Niche 优化（关键内存收益）

| 类型 | 64 位典型大小 | 原因 |
|------|---------------|------|
| `*mut T` | 8 B | 裸指针 |
| `Option<*mut T>` | **16 B** | 需额外 discriminant 表示 `None` |
| `NonNull<T>` | 8 B | 构造保证非 null |
| `Option<NonNull<T>>` | **8 B** | 复用 **0 地址** 编码 `None` — 与裸指针同大 |

> **总结**：确定指针一定非空时，优先 `NonNull<T>` — 比原生可空裸指针更安全，且 `Option<NonNull<T>>` 零开销。

→ Niche 详解 [Ch02 §03 复合类型](../Chapter-02-Types/03-complex-types.md) · DST / 宽指针 [Ch02 §04](../Chapter-02-Types/04-dst-wide-pointers.md)

---

## 四、裸指针 vs `NonNull` 对比示例

```rust
use std::mem::size_of;
use std::ptr::NonNull;

fn main() {
    let mut x = 42;

    // --- 原生裸指针 ---
    let raw: *mut i32 = &mut x;           // Safe：创建
    let read_only: *const i32 = raw;      // Safe：*mut → *const

    unsafe {
        *raw += 1;
        println!("via raw: {}", *read_only);
    }

    let null: *mut i32 = std::ptr::null_mut();
    assert!(null.is_null());              // 须手动判空

    // --- NonNull ---
    let nn = NonNull::from(&mut x);       // 引用来源，保证非空
    assert!(NonNull::new(std::ptr::null_mut()).is_none());

    unsafe {
        *nn.as_ptr() += 1;                // 解引用仍须 unsafe
    }

    // --- Niche 优化：Option 大小对比 ---
    println!("*mut i32:            {} B", size_of::<*mut i32>());
    println!("Option<*mut i32>:    {} B", size_of::<Option<*mut i32>>());
    println!("NonNull<i32>:        {} B", size_of::<NonNull<i32>>());
    println!("Option<NonNull<i32>>: {} B", size_of::<Option<NonNull<i32>>>());
    // 典型 64 位：8, 16, 8, 8
}
```

---

## 五、安全规范

| 原则 | 说明 |
|------|------|
| **默认不用裸指针** | 业务代码优先 `&` / `&mut`、`Box` / `Rc` / `Arc` |
| **底层才用** | 自定义数据结构、FFI、运行时内存管理、标准库内部 |
| **能 `NonNull` 就不用可空裸指针** | 减少空指针 UB + 获得 `Option` niche |
| **解引用必 `unsafe`** | 块前写 `// SAFETY:` 说明前提 → [11 文档](./11-documentation.md) |
| **有效性 ≠ 非 null** | 非 null 仍可能是悬垂/未初始化 → [06 validity](./06-validity.md) |

---

## 六、常见误区

| 误区 | 纠正 |
|------|------|
| 创建裸指针也要 unsafe | **只有解引用**（及少数 API）须 unsafe |
| `NonNull` 解引用是 Safe | 仍须 `unsafe { *nn.as_ptr() }` |
| 裸指针 = C 指针，随便用 | Rust 仍有 **validity / 别名 / 对齐** 等 UB 规则 |
| `Option<*mut T>` 和 `*mut T` 一样大 | 通常 **更大**；用 `Option<NonNull<T>>` |
| 有裸指针就能绕过借用检查写业务逻辑 | 应封装为 Safe API，缩小 unsafe 边界 → [10 管理边界](./10-manage-boundaries.md) |

---

## 七、口诀

- **裸指针**：无生命周期、无别名、可 null；**创建 Safe，解引用 unsafe**。  
- **三大场景**：自引用 + Pin、`Arc` 内部、FFI。  
- **非空优先 `NonNull<T>`**：更安全 + `Option` 不占额外字节。

---

## 自测

- [ ] 说明创建 `&x as *const T` 为何 Safe，而 `*ptr` 为何 unsafe  
- [ ] `Option<*mut i32>` 与 `Option<NonNull<i32>>` 在 64 位上大小差多少？为什么？  
- [ ] 自引用结构为何需要裸指针 + `Pin`，普通 `&` 为何不够？

→ 下一节：[03 调用 unsafe 函数](./03-calling-unsafe-functions.md)
