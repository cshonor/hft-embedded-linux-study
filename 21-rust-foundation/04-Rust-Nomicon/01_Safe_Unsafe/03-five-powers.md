# 三 · Unsafe Rust 独有的 5 种高危能力

← [本章目录](./README.md) · 上一节：[02-unsafe-contract.md](./02-unsafe-contract.md) · 下一节：[04-trust-and-nonlocality.md](./04-trust-and-nonlocality.md)

---

Safe Rust 完全禁止这五类操作；**任意一种违背约束 → 未定义行为**。仅能在 `unsafe {}` 块中执行（实现 unsafe trait 时用 `unsafe impl`）。

| # | 能力 | 要点 |
|:-:|------|------|
| 1 | 解引用裸指针 `*const T` / `*mut T` | 仅**解引用**需 unsafe；定义/赋值裸指针变量属于 Safe |
| 2 | 调用 `unsafe fn` | 含自定义 unsafe 方法、C FFI、`intrinsic` |
| 3 | 实现 `unsafe trait` | 如 `Send`/`Sync`；须 `unsafe impl` |
| 4 | 读写 `static mut` | Safe 中连读都不允许 |
| 5 | 访问 `union` 字段 | 编译器无法跟踪当前有效类型 |

## 1. 解引用裸指针

普通引用 `&T` / `&mut T` 受借用检查、生命周期约束；裸指针完全绕过编译器校验，可指向任意内存地址。

**例子**

```rust
let mut n = 42;
let ptr: *mut i32 = &mut n as *mut i32;  // ✅ Safe：创建裸指针
// let v = *ptr;                          // ❌ 解引用必须 unsafe

unsafe {
    *ptr += 1;                            // ✅ 解引用 + 写入
}
assert_eq!(n, 43);
```

**风险**：悬垂指针、越界访问、同时存在可变 + 不可变访问。

```rust
let ptr = 0xdeadbeef as *mut i32;
// unsafe { *ptr = 1; }  // UB：地址无效
```

→ 本 crate：`raw_pointer_and_unsafe_fn()` · [five_powers.rs](./src/five_powers.rs) · 深度解读：[07-raw-pointers.md](./07-raw-pointers.md)

## 2. 调用 unsafe fn

包含三类：自定义不安全方法、C 语言 FFI 外部函数、编译器内置 intrinsic 底层函数。

**规则**：无论函数有无返回值、参数多少，调用时必须包裹 unsafe 块。

**例子**

```rust
unsafe fn increment(ptr: *mut i32) { *ptr += 1; }

unsafe {
    increment(ptr);     // 自定义 unsafe fn
    libc::free(p);      // C FFI（ch09）
    std::ptr::read(p);  // 标准库里的 unsafe fn
}
```

HFT 场景：DPDK / 交易所 C API 的 `extern "C" fn` 声明后，每次调用都在 `unsafe { ... }` 里，外层再包一层 Safe 包装（如 `safe_read_tick()`）。

→ 本 crate：`increment` + `raw_pointer_and_unsafe_fn()`

## 3. 实现 unsafe trait

普通 trait 实现无需 unsafe；带 unsafe 标记的 trait，实现时强制加 `unsafe impl`。

**例子**

```rust
unsafe trait Marker {}
unsafe impl Marker for u32 {}   // 必须 unsafe impl

// Send / Sync 同理 — 错误实现 = 全程序 UB：
struct Bad(*mut u8);
unsafe impl Send for Bad {}     // 若 *mut 指向非 Send 数据，多线程即数据竞争
```

**风险**：错误实现 `Send`/`Sync` 会引发多线程数据竞争，破坏内存安全。

→ 本 crate：`unsafe_trait_demo()` · ch08 `MyVec` 的 `unsafe impl Send/Sync`

## 4. 读写 static mut

全局可变静态变量无借用保护、无线程同步，多线程同时读写必然数据竞争。

Safe 代码中连读取 `static mut` 都不允许，只能在 unsafe 块内操作。

**例子**

```rust
static mut COUNTER: u32 = 0;

fn bump() -> u32 {
    // COUNTER += 1;              // ❌ 读/写 static mut 都要 unsafe
    unsafe {
        COUNTER += 1;
        COUNTER
    }
}
```

生产代码更推荐 `AtomicU32` / `Mutex`（ch07）；`static mut` 多见于嵌入式全局状态或 demo。

→ 本 crate：`static_mut_demo()` · 连续调用 `cargo run` 可见 counter 递增

## 5. 访问 union 字段

Union 同一内存空间存放多种类型，编译器无法跟踪当前存储的有效类型，直接读取字段极易读取错误二进制数据，引发 UB。

**例子**

```rust
union IntOrFloat { i: u32, f: f32 }

let u = IntOrFloat { i: 0x3f800000 };  // 写入 i 字段
// let x = u.f;                          // ❌ 读任一字段都要 unsafe
unsafe {
    assert_eq!(u.i, 0x3f800000);       // 按 i 解释
    // u.f 此时是 reinterpret — 若按 f 读会得到 1.0，但类型语义由你保证
}
```

FFI 里 C 的 `union` 互操作常见此模式；必须文档约定「当前有效成员」。

→ 本 crate：`union_field_demo()` · `0x3f800000` 即 `f32` 的 `1.0` 位模式

---

## 对照表：哪些操作 Safe、哪些要 unsafe

| 操作 | Safe？ |
|------|:------:|
| `let p = &x as *const T` | ✅ |
| `*p` / `p.read()` | ❌ 需 `unsafe` |
| 调用普通 `fn` | ✅ |
| 调用 `unsafe fn` | ❌ 需 `unsafe` |
| `impl Trait for T`（普通 trait） | ✅ |
| `unsafe impl Send for T` | ❌ 需 `unsafe impl` |
| 读 `static X: T` | ✅（不可变静态） |
| 读/写 `static mut X` | ❌ 需 `unsafe` |
| 定义 `union U { ... }` | ✅ |
| 访问 `u.field` | ❌ 需 `unsafe` |

→ 源码逐项对照：[src/five_powers.rs](./src/five_powers.rs)

```bash
cd 04-Rust-Nomicon/01_Safe_Unsafe
cargo run
```
