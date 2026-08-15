# 1.3.1 Rust 内存模型（Safe Rust 三分类）

> 所属：**Talking About Memory** · [← 03 索引](./03-memory-regions.md) · [03.2 OS / LLVM 布局 →](./03-2-os-memory-layout.md)

**视角**：**Safe Rust + 所有权 / 借用** — 学 Book、写业务、理解 `Box` / `String` / 作用域时用。

与 [02 变量深入 · 高层数据流模型](./02-variables-in-depth.md) 同一层：关心**值在哪一类存储、谁拥有、何时 drop**，不必先画 OS 五分区图。

---

## 为什么 Rust 只分三类？

RFR / Book 把进程内存收成 **栈 / 堆 / 静态**，是因为 **safe Rust 的规则按这三类管**：

| Rust 分类 | 典型内容 | Safe Rust 怎么管 |
|-----------|----------|------------------|
| **栈 (stack)** | `let` 局部变量、函数参数、调用帧 | 进函数分配、出函数销毁；与**作用域**对齐 |
| **堆 (heap)** | `Box` / `Vec` / `String` 的 **payload** | **所有权** + `Drop`；无 GC |
| **静态 (static)** | `static` 全局；`'static` 引用可指向的数据 | 进程全程；`static mut` 要 `unsafe` |

OS 还有 Text / Data / BSS / mmap 等 — 那是 [03.2](./03-2-os-memory-layout.md) 的事。日常写 safe 代码，**三分类足够**。

---

## 栈：自动、快、有上限

```rust
fn foo() {
    let x = 42;           // x 在 foo 的栈帧里
    let arr = [0u8; 1024]; // 数组也在栈（若太大可能改放栈外，见下方）
} // foo 返回 → 整帧弹出，x、arr 自动销毁
```

| | |
|---|---|
| **分配 / 释放** | 编译器 + 调用约定：压栈 / 弹栈 |
| **速度** | 极快（改栈指针） |
| **大小** | 每线程固定上限 → 递归过深 **stack overflow** |
| **和借用的关系** | 借用检查里的「作用域」多与栈帧生命周期对齐 |

**大对象**：非常大的栈数组可能触发 **stack overflow**；超大缓冲常放堆（`Vec`）或静态。

---

## 堆：动态、句柄在栈、实体在堆

```rust
fn main() {
    let b = Box::new(100);              // b（句柄）在栈；100 在堆
    let s = String::from("hello");      // s（{ptr,len,cap}）在栈；字节在堆
    let v = vec![1, 2, 3];              // 同理
} // drop 顺序：先释放堆 payload，再弹栈
```

| | 栈上的「句柄」 | 堆上的 payload |
|---|----------------|----------------|
| `Box<T>` | `Box` 结构体（指针） | `T` |
| `String` | `{ ptr, len, cap }` | UTF-8 字节 |
| `Vec<T>` | `{ ptr, len, cap }` | `T` 元素数组 |

**所有权在做什么**：栈上变量（如 `b: Box<i32>`）**拥有**堆块；离开作用域 → **`Drop` 释放堆**，避免泄漏。→ [04 所有权](./04-ownership.md)

| | 栈 | 堆 |
|---|-----|-----|
| 分配 / 释放 | 自动（进/出函数） | `Box::new` 等 + **drop** |
| 速度 | 极快 | 较慢（分配器、可能向 OS 要页） |
| 典型 Rust | `let x = 42;` | `Box::new(100)` 里的 `100` |

---

## 静态：`static` 与 `'static`

```rust
static GLOBAL: i32 = 100;       // 进程全程存活
static mut COUNTER: i32 = 0;    // 可改须 unsafe

fn main() {
    let r: &'static str = "hi"; // 字面量；数据常在只读段（见 03.2）
}
```

| | |
|---|---|
| **`static`** | 编译期已知地址；活整个进程 |
| **`static mut`** | 须 **`unsafe`**；多线程还要防数据竞争 → 优先 `OnceLock` 等 |
| **`'static` 生命周期** | **≠** 「数据一定在 Data/BSS」：owned 的 `String` 也可 `T: 'static`，人可能在**堆**上 |

→ [08 生命周期](./08-lifetimes.md)

---

## 串起来：段 ↔ 所有权 / 借用

| 分类 | Rust 关系 |
|------|-----------|
| **栈** | 局部绑定、调用帧；`let` / 参数 / 返回值移动 |
| **堆** | owned 数据；栈上常是**小句柄** |
| **静态** | `static`；与 `'static` 生命周期相关但不等价 |

**和 02 双模型的对应**：

| 02 模型 | 03.1 三分类 |
|---------|-------------|
| **高层 · 数据流** | 主战场：谁拥有、谁借用、何时失效 |
| **底层 · 内存槽** | 槽在栈还是静态区；堆句柄在栈、payload 在堆 |

---

## 完整示例（Safe Rust 视角）

```rust
static GLOBAL_INIT: i32 = 100;

fn main() {
    let a = 42;              // 栈
    let b = Box::new(100);   // b 在栈；100 在堆

    let r = &a;              // 借用栈上 a
    println!("{r} {GLOBAL_INIT}");
} // b drop → 堆释放；main 帧弹出
```

| 符号 / 数据 | Rust 三分类 |
|-------------|-------------|
| `a`、`b`（Box 句柄） | **栈** |
| `*b` 的 `100` | **堆** |
| `GLOBAL_INIT` | **静态** |
| `main` 的机器码 | 不在三分类里 → [03.2 · Text](./03-2-os-memory-layout.md) |

---

## 何时读 03.2？

| 你在做… | 读 |
|---------|-----|
| 学所有权、写 `Vec` / `String` | **本节 03.1** |
| 读 LLVM IR、`static` 进 Data 还是 BSS | [03.2](./03-2-os-memory-layout.md) |
| FFI、`mmap`、HFT 看 `/proc/maps` | [03.2](./03-2-os-memory-layout.md) |

→ [04 所有权](./04-ownership.md) · [02 变量深入](./02-variables-in-depth.md)
