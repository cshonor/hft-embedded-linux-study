# 3.1 Shared References（共享引用 `&T`）

> 所属：**Borrowing and Lifetimes** · [← 章索引](./README.md)

前置 → [04 所有权 · Move/Copy/Drop](./04-ownership.md) · 术语 → [01 内存术语](./01-memory-terminology.md)

---

## 核心语义：共享引用是什么

**共享引用 `&T` = 只读借用** — 在引用存活期内：

- 可以同时存在**多个** `&T` 指向同一数据（多读）
- **不能**同时存在 `&mut T` 指向同一数据
- **不能**通过所有者或其它合法路径**直接修改**该内存（除非走 [07 内部可变性](./07-interior-mutability.md)）

这就是 **单写多读（Single Writer, Multiple Readers）** 规则的一部分 — Rust 内存安全与优化的基石。

```rust
let x = 42;
let r1 = &x;
let r2 = &x;     // ✅ 多个 &T 共存
println!("{r1} {r2}");
// let m = &mut x; // ❌ 已有共享借用时不能再 &mut
```

**大白话**：`&T` 是「我借来看，大家都可以看，但谁也不能在这段时间里改（常规路径）」。

---

## 别名组合规则（一眼表）

对**同一内存位置**在同一时刻：

| 组合 | 是否允许 |
|------|----------|
| 多个 `&T` | ✅ |
| 一个 `&mut T` | ✅ |
| `&T` + `&mut T` | ❌ |
| 多个 `&mut T` | ❌ |

→ 可变借用详见 [06 可变引用](./06-mutable-references.md)

**引用不重叠**：编译期借用检查器 enforcement；违反则编译失败。

---

## 与 LLVM / 优化器

共享引用不只是语法 — 也是给优化器的**契约**：

| 借用 | 向优化器承诺 | LLVM 可做什么 |
|------|-------------|---------------|
| **`&T`** | 存活期内**不会被突变**（no-mutation / readonly 语义） | 重复读可合并、缓存到寄存器；死代码消除、循环不变量提升等 |
| **`&mut T`** | **独占**、无别名冲突（noalias） | 更激进的指针别名分析与 store/load 优化 |

```rust
fn sum_twice(r: &i32) -> i32 {
    *r + *r   // 优化器可能只从内存读一次 *r
}
```

**为何重要（HFT / 性能）**：安全 Rust 里写 `&T`，编译器能假定「这段期间没人改」— 不必每次读都当成可能 alias 的 wild pointer。读 IR 时见 [06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17 ch04](../../06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/part02_src_to_machine/chapter04_ir_basic/README.md)。

**与内部可变性的关系**：`RefCell` / `Mutex` 等**刻意** opt-out 普通 `&T` 的 no-mutation 假设 — 见 [07](./07-interior-mutability.md)，否则 LLVM 会按「只读」做**错误**优化。

---

## `&T` vs C 的 `const T*`

| | **`&T`（Rust）** | **`const T*`（C）** |
|---|------------------|---------------------|
| 只读访问 | ✅ | ✅ |
| **生命周期** | ✅ 编译期检查 | ❌ 无 |
| **别名规则** | ✅ 与 `&mut` 互斥 | ❌ 可随意 cast 成 `T*` |
| 悬垂 | 能编译通过则通常安全 | 全靠程序员 |

Rust 的 `&T` **不是**「带类型的 const 裸指针」— 还编码**能借多久、能和谁共存**。

---

## 内部可变性：看起来是例外

外表可以是 **`&RefCell<T>`**（共享引用），内部仍可在**运行时**借 mut — 规则由 `RefCell` 自己检查，违反则 **panic**。

```rust
use std::cell::RefCell;

let data = RefCell::new(5);
let r = &data;              // 共享引用 RefCell 容器本身

*r.borrow_mut() += 1;       // 运行时独占 borrow，改内部
println!("{}", r.borrow()); // 6
```

```rust
// 多个 & 指向同一 RefCell 可以，但不能同时 borrow_mut
let data = RefCell::new(5);
let r1 = &data;
let r2 = &data;

{
    let _a = r1.borrow_mut(); // 持有 mut borrow
    // let _b = r2.borrow_mut(); // ❌ 运行时 panic：已有 mut borrow
}
```

- **编译期**：多个 `&RefCell<T>` 合法  
- **运行时**：`borrow` / `borrow_mut` 仍互斥 — 与 [07 内部可变性](./07-interior-mutability.md) 一致

多线程 → `Mutex<T>` / `RwLock<T>` 等同理，见 [第 10 章 并发](../Chapter-10-Concurrency-and-Parallelism/README.md)。

---

## 常见误区

| 误区 | 纠正 |
|------|------|
| 「有 `&T` 就完全不能改」 | **直接改**不行；**内部可变性**（`RefCell` / `Mutex` / `Cell`）是显式例外 |
| 「有 `&mut` 就完全不能改」 | `&mut` **就是**改的路径；不能的是**同时**再有冲突的 `&` / `&mut` |
| 「`&T` = C 的 `const T*`」 | 还有**生命周期 + 别名协议** |
| 「多个读者 = 可以随便改内部」 | 只有 **UnsafeCell 系**封装在规则内改；普通 `&T` 到 `i32` 不能改 |
| 「优化与借用无关」 | `&T` / `&mut T` 的假设直接喂给 LLVM |

---

## 和所有权的关系

| | 所有者 `T` | 共享引用 `&T` |
|---|------------|---------------|
| 拥有数据 | ✅ | ❌（只借看） |
| 触发 Drop | ✅ | ❌（只结束借用） |
| 赋值 / move | 转移所有权 | **Copy**（复制指针 + 生命周期） |

→ [04 所有权](./04-ownership.md) · 下一节 [06 可变引用 `&mut T`](./06-mutable-references.md)

---

## 对照阅读

- Book → [4.2 引用与借用](../../00-Book/04-ownership/4.2-引用与借用.md)
- 内部可变性 → [07 Interior Mutability](./07-interior-mutability.md)
- ER → [Item 08 引用与指针](../../01-ER/Chapter-01-Types/Item-08-references-pointers/README.md)
