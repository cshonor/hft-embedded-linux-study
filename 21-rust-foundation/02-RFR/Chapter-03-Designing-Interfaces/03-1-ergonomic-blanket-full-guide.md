# 1.3.1 · 人体工程学 Trait 实现完整解读（Blanket impl）

← [03 人体工程学 impl](./03-ergonomic-trait-implementations.md)

> **归属**：Unsurprising · ER [Item 13 默认实现](../../01-ER/Chapter-02-Traits/Item-13-default-implementations/README.md)  
> **目标**：消除调用分叉 — 引用 / 智能指针可直接调用**自定义 trait** 方法。

Demo → [`blanket-trait-demo/`](./blanket-trait-demo/) · `cargo run --manifest-path blanket-trait-demo/Cargo.toml`

---

## 一、问题背景：无 blanket 的糟糕体验

Rust 所有权与引用分离时，若只为 `T` impl trait，`&T` / `Box<T>` 等**不能**直接调用 — 用户须手动 `*` 解引用，形成**分叉 API**，违背 Unsurprising。

### 反例

```rust
trait MyTrait {
    fn work(&self);
}
struct Foo;
impl MyTrait for Foo {
    fn work(&self) { println!("ok"); }
}

fn main() {
    let f = Foo;
    f.work();     // ✅
    let rf = &f;
    // rf.work(); // ❌ &Foo 未实现 MyTrait
    (*rf).work(); // 用户被迫手动解引用
}
```

---

## 二、Blanket impl 是什么：两种形态

**Blanket impl** = 不给某个**具体类型**单独写 impl，而是写一条**泛型规则**：「凡满足某条件的类型，统一获得某 trait 实现」。核心目的是**消除重复 impl**。

| 形态 | 写法 | impl 落在谁身上 | 典型目的 |
|------|------|-----------------|----------|
| **A. 条件补能力** | `impl<T: SomeOtherTrait> MyTrait for T` | **T 本身** | 「实现了 A 就自动有 B」 |
| **B. 包装转发** | `impl<T: MyTrait + ?Sized> MyTrait for &T` | **`&T` / `Box<T>` 等** | 引用/指针也能调，不用手动 `*` |

```text
形态 A：  T: SomeOtherTrait  ──自动──►  T: MyTrait
形态 B：  T: MyTrait         ──自动──►  &T / Box<T>: MyTrait（内部转发到 T）
```

### 形态 A 示例 — 「有 Debug 就能打日志」（概念）

```rust
trait Loggable {
    fn log(&self);
}

// 凡实现了 Debug 的 T，自动获得 Loggable
impl<T: std::fmt::Debug> Loggable for T {
    fn log(&self) {
        println!("{self:?}");
    }
}
```

一行代替「给每个 struct 各写一份 impl」。⚠️ 但 bound 绑 **`Debug` 这种全域 trait 过宽**，易与未来 impl 冲突 — 见 [§五·2](#2-过宽-blanket--版本兼容风险)；生产里更常**收窄**到业务 marker trait。

### 形态 B 示例 — 「有 MyTrait 的引用也能 work」（RFR §03 重点）

不是给 `Foo` 再加方法，而是给 **trait 做能力补全**：凡 `T: MyTrait`，其 **`&T`、`Box<T>`** 也自动 `MyTrait`，内部转发：

```rust
impl<T: MyTrait + ?Sized> MyTrait for &T {
    fn work(&self) { (*self).work() }
}
```

```text
只 impl Foo: MyTrait  →  只有 f.work()
+ blanket for &T      →  (&f).work() 也合法，无需 (*rf).work()
```

**形态 B 不改变「谁实现了 MyTrait」的定义，只消除值 / 引用 / 智能指针的调用分叉。**

---

## 三、两套并行逻辑：结构体 impl ≠ blanket impl

给 **`Foo` 写 impl** 和写 **blanket impl** 是两件独立的事 — 做前者时**完全不用**考虑后者。

| | **具体 impl** | **blanket impl** |
|---|---------------|------------------|
| **写法** | `impl MyTrait for Foo { … }` | `impl<T: MyTrait> MyTrait for &T { … }` |
| **`for` 后面** | 一个**具体类型** `Foo` | **类型模式** `&T`、`Box<T>`、`T`（带 where） |
| **谁写** | 每个业务类型写**一次**核心逻辑 | trait 作者写**一条规则**，编译器批量补 |
| **关注点** | 「Foo 的 `work` 干什么」 | 「包装形式能不能调 `work`」 |

```text
第一步（必做）     impl MyTrait for Foo     →  Foo 有 MyTrait
第二步（可选补丁）  impl MyTrait for &T      →  &Foo、Box<Foo>… 也有 MyTrait（转发到内层）
```

**blanket 可以事后补** — 核心类型 impl 稳定后，再加一层 `&T` / `Box<T>` 转发，**不改** `Foo` 里已有逻辑。

### 何时真的需要形态 B？

具体变量上 `rf.work()`（`rf: &Foo`）有时能靠方法调用的 **autoref** 通过，但**泛型 bound** 要求 `T: MyTrait` 时，传 `&Foo` / `Box<Foo>` 会要求 **该包装类型本身** impl 了 trait：

```rust
fn generic<T: MyTrait>(t: T) { t.work(); }

let f = Foo;
let rf = &f;
rf.work();        // ✅ 常能通过（autoref 到 Foo 的 &self）
generic(f);       // ✅ T = Foo
generic(rf);      // ❌ T = &Foo，未 impl MyTrait — 除非有 blanket for &T
generic(Box::new(Foo)); // ❌ 同理
```

→ demo 源码：[blanket-trait-demo/src/lib.rs](./blanket-trait-demo/src/lib.rs)（泛型 `T: MyTrait` 须 blanket，见 [03-1 §三](./03-1-ergonomic-blanket-full-guide.md#三两套并行逻辑结构体-impl--blanket-impl)）

---

## 四、形态 B 详解：包装转发模板

为 **引用、可变引用、常见智能指针** 批量 impl，内部转发到内层 `T`。

### 标准模板：`&T` / `&mut T`

```rust
trait MyTrait {
    fn work(&self);
}

impl<T: MyTrait + ?Sized> MyTrait for &T {
    fn work(&self) { (*self).work() }
}

impl<T: MyTrait + ?Sized> MyTrait for &mut T {
    fn work(&self) { (**self).work() }
}
```

`?Sized`：允许 `T` 为 DST（`str`、`[u8]`、`dyn Trait`），使 `&str` 等能命中 blanket。

→ 详解：[03-2 `?Sized` 与 `?`](./03-2-question-sized.md)

### 扩展：`Box` / `Arc` / `Rc`

```rust
use std::{boxed::Box, rc::Rc, sync::Arc};

impl<T: MyTrait + ?Sized> MyTrait for Box<T> {
    fn work(&self) { (**self).work() }
}
impl<T: MyTrait + ?Sized> MyTrait for Arc<T> {
    fn work(&self) { (**self).work() }
}
impl<T: MyTrait + ?Sized> MyTrait for Rc<T> {
    fn work(&self) { (**self).work() }
}
```

### 使用效果（无分叉）

```rust
let mut f = Foo;
f.work();
(&f).work();
(&mut f).work();
Box::new(Foo).work();
Arc::new(Foo).work();
```

→ demo 源码：[blanket-trait-demo/src/lib.rs](./blanket-trait-demo/src/lib.rs)

---

## 五、两大约束

### 1. 孤儿规则 & 相干性 — 仅**自己的 trait**

| 情况 | 能否 blanket |
|------|:------------:|
| **本 crate 定义的 trait** | ✅ |
| **外部 trait**（如 `std::fmt::Debug`） | ❌ 不能给 `&T` 批量 impl |
| 外部已有同范围 blanket | ❌ 重复 impl，编译失败 |

**结论**：blanket 人体工程学优化**只适用于自定义 trait**；第三方 trait 用**扩展 trait**（extension trait）等模式。

→ [07 相干性与孤儿规则](../Chapter-02-Types/07-coherence-orphan-rule.md)

### 2. 过宽 blanket — 版本兼容地雷

```rust
// ❌ 不推荐：未来任何类型单独 impl MyTrait 都可能冲突
impl<T: std::fmt::Debug> MyTrait for T {}
```

| 风险 | 说明 |
|------|------|
| 依赖库为某类型 impl `MyTrait` | 相干性冲突 |
| 本 crate 后续再加 blanket | 与旧 impl 重叠 |

**规避**：

1. **收窄 bound** — 只限定业务内 marker trait，别绑全域 `Debug`；
2. **[Sealed trait](./12-trait-implementations.md)** — 锁 impl 集合，blanket 范围可控。

---

## 六、ER Item 13：默认实现（双管齐下）

| 端 | 手段 |
|----|------|
| **实现者** | 默认方法，少写样板 |
| **调用者** | blanket 让 `&T` 也能调全部方法 |

```rust
trait Count {
    fn len(&self) -> usize;
    fn is_empty(&self) -> bool {
        self.len() == 0
    }
}

impl<T: Count + ?Sized> Count for &T {
    fn len(&self) -> usize {
        (**self).len()
    }
}
// &items.is_empty() 合法
```

---

## 七、标准库佐证

| 模式 | 形态 | 例子 |
|------|------|------|
| 条件补能力 | **A** | `impl<T: Display> ToString for T` |
| 包装转发 | **B** | `impl<T: Clone + ?Sized> Clone for &T`（复制引用本身） |
| `From` / `Into` | **A** | 实现 `From` 自动获得 `into()` |

std 能这么写是因为 trait 与 impl 都在**同一 crate（std）**，不受孤儿规则限制。

---

## 八、可复制模板（含 Sealed）

### A. 最小 blanket（`&T` + `&mut T`）

```rust
pub trait MyTrait {
    fn work(&self);
}

impl<T: MyTrait + ?Sized> MyTrait for &T {
    fn work(&self) { (*self).work() }
}
impl<T: MyTrait + ?Sized> MyTrait for &mut T {
    fn work(&self) { (**self).work() }
}
```

### B. 完整转发（+ `Box` / `Arc` / `Rc`）

见 [blanket-trait-demo/src/lib.rs](./blanket-trait-demo/src/lib.rs) 中 `blanket_impls!` 宏或手写 impl 块。

### C. Sealed + blanket（长期稳定 API）

```rust
mod sealed {
    pub trait Sealed {}
}

pub trait MyTrait: sealed::Sealed {
    fn work(&self);
}

impl sealed::Sealed for MyType {}
impl MyTrait for MyType {
    fn work(&self) { /* … */ }
}

// 仅 MyType（及本 crate 内 impl Sealed 的类型）能 impl MyTrait
// 再对 &T / Box<T> 做 blanket 转发，范围可控
impl<T: MyTrait + ?Sized> MyTrait for &T {
    fn work(&self) { (*self).work() }
}
```

→ Sealed 详述：[12 Trait 实现控制](./12-trait-implementations.md)

---

## 九、落地清单

1. 自定义 trait → 优先补 **`&T` / `&mut T` blanket**，带 **`?Sized`**；
2. 按需补 **`Box` / `Arc` / `Rc`**；
3. **禁止**无边界 `impl<T: Debug> MyTrait for T`；
4. 长期公开 API → 考虑 **sealed trait**；
5. trait 内用**默认方法**（Item 13）减 impl 负担；
6. **外部 trait** 不能 blanket → 用 extension trait。

---

## 速记

**两套并行**：Foo 只写 `impl MyTrait for Foo` · blanket 事后补 · `for` 后是类型模式 `&T`  
**模板**：`impl<T: MyTrait + ?Sized> MyTrait for &T` · 仅**自己的 trait** · 可 sealed · ER Item 13 默认方法

→ 下一节：[04 包装类型](./04-wrapper-types.md)
