# 1. The unsafe Keyword（unsafe 关键字）

> 所属：**Unsafe Code** · [← 章索引](./README.md) · 下一节 [02 裸指针](./02-raw-pointers.md)

> 对应 **The Book [19.1 不安全 Rust](../../00-Book/19-advanced-features/19.1-不安全Rust.md)** · Nomicon [五种 unsafe 能力](../../04-Rust-Nomicon/01_Safe_Unsafe/03-five-powers.md)

---

## 零、核心总览

`unsafe` 在 Rust 里承担**双重角色**，有明确的权限边界 — **不会**废除所有安全检查。

```text
unsafe fn     → 立规矩：调用方须满足额外前提
unsafe { }    → 办险事：作者自行保证块内不变量
借用检查      → 全程在线，不因 unsafe 关闭
```

---

## 一、双重角色

| 写法 | 定位 | 完整含义 |
|------|------|----------|
| **`unsafe fn`** | **声明契约** | 函数带有安全前提：调用前调用者须手动满足额外规则；违背约定 → **未定义行为（UB）**，编译器不再兜底。<br>例：裸指针操作、FFI 外部函数、按偏移读内存等。 |
| **`unsafe { 代码块 }`** | **履行契约** | 块内可使用 Rust 默认封禁的「超能力」；**写代码的人**须保证内存安全、遵守契约、不产生 UB。<br>只有进入 `unsafe {}`，才能执行下列五类高危操作。 |

### 通俗理解

1. **`unsafe fn`**：我这个函数有使用门槛，乱用会 UB — 调用前必须看懂规范。  
2. **`unsafe {}`**：我确认已遵守规范，现在要执行高危操作，安全我自己负责。

### `unsafe fn` 与块的关系

| 误区 | 正解 |
|------|------|
| `unsafe fn` 内部一定有 `unsafe {}` | **不一定** — `unsafe fn` 约束的是**调用方**；函数体可以是安全代码，风险在「谁调用、是否满足前提」 |
| 进了 `unsafe fn` 就自动能解引用裸指针 | 函数**内部**若要做五类操作，仍须自己的 `unsafe {}`（除非已在另一个 unsafe 上下文中） |

```rust
// 声明契约：调用 increment 的人须保证 ptr 有效且别名规则满足
unsafe fn increment(ptr: *mut i32) {
    *ptr += 1;  // 函数体里解引用，此处本身就在 unsafe fn 体内 — 仍属 unsafe 上下文
}

unsafe fn metadata() -> &'static str {
    "ok"  // 函数体可以全是安全代码；危险在「调用方是否满足契约」
}
```

→ 调用细节 [03 调用 unsafe 函数](./03-calling-unsafe-functions.md)

---

## 二、关键：`unsafe` 不会让借用检查失效

### 1. 借用检查依然生效

`&T` 共享引用、`&mut T` 可变引用的**生命周期、读写互斥**规则 — **哪怕写在 `unsafe {}` 内部，编译器依旧检查**。

`unsafe` 只放开特定高危操作权限，**不关闭**基础借用安全校验。

```rust
let mut x = 10;
let r = &x;
unsafe {
    // let m = &mut x;  // ❌ 仍有活跃 &x，编译报错 — 与是否在 unsafe 块无关
    println!("{}", r);
}
```

### 2. 仅放开以下 5 类高危操作

只有这些操作，必须包裹在 `unsafe {}` 中才能执行（与 Book 19.1 一致）：

| # | 操作 | 一句话 |
|:-:|------|--------|
| 1 | 解引用裸指针 `*const T` / `*mut T` | 创建裸指针可在 Safe 代码；**解引用**须 unsafe |
| 2 | 调用 `unsafe fn` / unsafe 方法 | 含 FFI、`std::ptr::write` 等 |
| 3 | 读写 `static mut` | Safe 代码连读都不允许 |
| 4 | `unsafe impl` 不安全 trait | 如手动 `Send` / `Sync` |
| 5 | 访问 `union` 字段 | 编译器无法跟踪当前有效变体 |

**除此之外**的普通代码，哪怕写在 `unsafe {}` 里，依旧受完整安全规则约束。

→ 逐项展开 [02 裸指针](./02-raw-pointers.md) · [04 unsafe trait](./04-unsafe-traits.md) · Nomicon [03-five-powers](../../04-Rust-Nomicon/01_Safe_Unsafe/03-five-powers.md)

---

## 三、与内部可变性：`RefCell` / `UnsafeCell` 怎么用 unsafe

标准库把 `unsafe` **封在库内部**，对外暴露 Safe API — 这是你学过 [Ch01 §07 UnsafeCell](../Chapter-01-Foundations/07-2-unsafecell-and-containers.md) 的 unsafe 侧：

```text
你调用 RefCell::borrow_mut()     →  Safe 接口
        ↓
标准库 unsafe { *UnsafeCell::get(...) }  →  库作者承担证明责任
        ↓
BorrowFlag 计数器保证无并发 Ref + RefMut
```

| 谁写 unsafe | 典型 |
|-------------|------|
| 标准库作者 | `Cell` / `RefCell` / `Vec` 扩容等内部 |
| 应用开发者 | 直接操作裸指针、FFI、`static mut`、自定义 `unsafe impl` |

**结论**：调用 `.borrow()` / `.set()` **不需要**你写 `unsafe`；只有你亲自做五类操作时，才须 `unsafe {}`。

---

## 四、常见误区

| 误区 | 纠正 |
|------|------|
| 写了 `unsafe {}` 就能随便写，借用检查失效 | 借用、生命周期**照常**；只解锁五类高危操作 |
| `unsafe fn` 内部一定有 `unsafe {}` | `unsafe fn` 约束**调用方**；函数体可以是安全代码 |
| unsafe = 完全不安全 | 只是把证明责任从编译器转到开发者；严守契约仍可内存安全 |
| unsafe 关掉所有检查 | **错** — 类型、借用、生命周期、大部分规则仍在 |
| 标准库 Safe API 里没有 unsafe | 大量 Safe API **内部**有 `unsafe`；边界在库内 |

---

## 五、极简口诀

- **`unsafe fn`**：立规矩，调用要守规矩。  
- **`unsafe {}`**：办险事，写代码的人兜底安全。  
- **借用检查全程在线**；unsafe 只解锁固定 **5 类**操作。

---

## 六、本章阅读路线

```text
01 unsafe 关键字（本节）→ 02 裸指针 → 03 调用 unsafe fn → 04 unsafe trait
→ 05–09 责任（UB/validity/panic/转换/drop）
→ 10–12 边界/文档/Miri
```

## 自测

- [ ] 说出 `unsafe fn` 与 `unsafe {}` 各约束谁  
- [ ] 在 `unsafe {}` 里写 `let m = &mut x`  while `&x` 活跃 — 会编译失败吗？  
- [ ] 列举五类须 unsafe 的操作；`RefCell::borrow` 算不算？  
- [ ] 为何 `UnsafeCell` 在标准库里、而你调用 `get`/`borrow` 不用写 unsafe？

→ 下一节：[02 裸指针](./02-raw-pointers.md)
