# 2.5.4 · 自动推导与 `unsafe impl`

> 所属：**Traits and Trait Bounds · 标记 Trait** · [← 09 hub](./09-marker-traits.md)

← [09.3 Send/Sync/Unpin](./09-3-send-sync-unpin.md) · 下一节 [10 存在类型](./10-existential-types.md)

---

## 一、Auto Trait 自动推导

`Send`、`Sync`、`Sized`（对具体类型）、`Unpin` 等由编译器**扫描所有字段**推导：成员都满足 → 自动实现，无需手写 `impl`。

---

## 二、手动 `unsafe impl`

`Send`、`Sync` 是 **unsafe trait**（实现须 `unsafe impl`）：

```rust
struct MyRawPtrWrapper {
    ptr: *mut u8,
}

// 仅当你能证明跨线程安全时才写：
unsafe impl Send for MyRawPtrWrapper {}
unsafe impl Sync for MyRawPtrWrapper {}
```

| 项 | 说明 |
|----|------|
| **场景** | FFI 裸指针包装、自定义无锁结构、跨线程 OS 句柄 |
| **含义** | 开发者向编译器担保线程安全；违反 → **UB** |
| **为何 unsafe trait** | 错误 impl 时**安全代码**即可数据竞争 → [Ch09 §04 unsafe trait](../../Chapter-09-Unsafe-Code/04-unsafe-traits.md) |
| **孤儿规则** | 仍须类型或 trait 至少一方本地 → [07.1](./07-1-orphan-rule.md) |

`Copy`、`Sized` 几乎不手动 `unsafe impl` — 用 `#[derive(Copy, Clone)]` 或语言内置规则。

---

## 三、核心设计目的

1. **编译期安全隔离** — `Send`/`Sync` 拒绝不安全跨线程类型，根源防数据竞争  
2. **内存布局约束** — `Sized` / `?Sized` 区分固定大小与 DST  
3. **所有权语义** — `Copy` 区分隐式位拷贝与 move  
4. **零开销抽象** — 无运行时代码，检查全在编译期  

---

## 四、易混区分

| 易混 | 纠正 |
|------|------|
| Marker = 没 trait 方法 | `Iterator` 等有方法是**行为** trait；`Send` 是**契约** marker |
| `Sync` = 可以多线程写 | `Sync` 管 **`&T` 共享读**；写要用 `Mutex` 等 |
| 手动 `Send` 就能乱传指针 | 仍须保证指向内存在线程间合法 |
| Marker 不受孤儿规则约束 | **同样约束** |
| `Copy` 与 `Clone` 可互换 | `Copy` 隐式浅拷贝；`Clone` 显式、可堆复制 |

---

## 对照阅读

- Book → [16.4 Send 与 Sync](../../00-Book/16-fearless-concurrency/16.4-Send与Sync.md)
- RFR → [第 10 章 并发](../Chapter-10-Concurrency-and-Parallelism/10-并发与并行-Concurrency-and-Parallelism-深度解析.md) · [第 8 章 Async](../Chapter-08-Asynchronous-Programming/README.md)
- ER → [Item 10 标准 trait](../../01-ER/Chapter-02-Traits/Item-10-standard-traits/README.md)
- Nomicon → [07 Concurrency](../../04-Rust-Nomicon/07_Concurrency_Atomic/README.md)
- Ch09 → [04 unsafe trait 精读](../../Chapter-09-Unsafe-Code/04-unsafe-traits.md)

→ 下一节：[10 存在类型](./10-existential-types.md)
