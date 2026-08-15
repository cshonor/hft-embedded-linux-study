# 2.5.3 · `Send` · `Sync` · `Unpin`

> 所属：**Traits and Trait Bounds · 标记 Trait** · [← 09 hub](./09-marker-traits.md)

← [09.2 Sized 与 Copy](./09-2-sized-copy.md) · 下一节 [09.4 unsafe impl](./09-4-unsafe-impl.md)

---

## 一、`Send`

| 项 | 说明 |
|----|------|
| **契约** | 类型的**所有权**可安全**转移**到另一线程 |
| **自动** | 成员均 `Send` 的复合类型通常自动 `Send` |
| **反例** | 裸指针 `*mut T`、`Rc<T>`（非原子 refcount）→ **非** `Send` |

```rust
use std::rc::Rc;
use std::thread;

let r = Rc::new(1);
// thread::spawn(move || { r.clone(); }); // ❌ Rc 非 Send
```

---

## 二、`Sync`

| 项 | 说明 |
|----|------|
| **契约** | `&T` 可安全地在多线程间**共享**（并发读） |
| **等价** | `T: Sync` ⟺ `&T: Send` |
| **反例** | `RefCell<T>`（运行时内部可变、非线程安全）→ **非** `Sync` |
| **多线程写** | 用 `Mutex<T>` / `RwLock<T>` 等（`T: Send` 时常可 `Sync`） |

```rust
use std::cell::RefCell;
// Arc::new(RefCell::new(0)); // RefCell 非 Sync
```

→ 并发：[RFR 第 10 章](../Chapter-10-Concurrency-and-Parallelism/README.md) · Nomicon [07 Send/Sync](../../04-Rust-Nomicon/07_Concurrency_Atomic/02-send-sync.md)

---

## 三、`Unpin`（异步 / `Pin`）

| 项 | 说明 |
|----|------|
| **契约** | 类型可安全**移动**内存（非「自引用结构」需钉住） |
| **默认** | 绝大多数类型自动 `Unpin` |
| **例外** | 部分 `async` 状态机内部可能 `!Unpin` |
| **用途** | `Pin<&mut T>` 体系；`!Unpin` 时不能随意 `mem::swap` 移动 |

→ [RFR 第 8 章 Async](../Chapter-08-Asynchronous-Programming/README.md)

---

## 四、五大标记对照总表

| Trait | 问的是什么 | 典型反例 / 注意 |
|-------|------------|-----------------|
| `Sized` | 编译期固定大小？ | `str`、`[T]`、`dyn Trait` |
| `Copy` | 赋值 = 位拷贝且原值仍有效？ | `String`、`Vec` |
| `Send` | 能 `move` 到别的线程？ | `Rc`、裸指针包装 |
| `Sync` | 能跨线程共享 `&T`？ | `RefCell`、`Cell` |
| `Unpin` | 能安全移动（非钉住自引用）？ | 部分 async 内部状态 |

→ 下一节：[09.4 自动推导与 `unsafe impl`](./09-4-unsafe-impl.md)

---

## 速记

## 五大标记

| Trait | 一句话 |
|-------|--------|
| `Sized` | 编译期固定 `size_of`；`?Sized` 放开 DST |
| `Copy` | 位拷贝，原值仍有效 |
| `Send` | 所有权可移到另一线程 |
| `Sync` | `&T` 可跨线程共享（`&T: Send`） |
| `Unpin` | 可安全移动（非 Pin 钉住） |

## 三句话

1. **`Sized`：默认要；`?Sized` 给 DST。**  
2. **`Copy` 隐式位拷贝；`Send` 移线程；`Sync` 共享 `&T`。**  
3. **`Send`/`Sync` 手动 impl 必须 `unsafe` + 孤儿规则。**

