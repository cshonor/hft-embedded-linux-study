# 2.3 Implementing Unsafe Traits（unsafe trait）

> 所属：**Great Power** · [← 章索引](./README.md)

← [03 调用 unsafe 函数](./03-calling-unsafe-functions.md) · 下一节 [05 什么会出错](./05-what-can-go-wrong.md)

前置 → [01 unsafe 五类超能力](./01-unsafe-keyword.md) · 标记 trait 速览 [Ch02 §09.3 Send/Sync](../Chapter-02-Types/09-3-send-sync-unpin.md)

> 对标：Book [19.1](../../00-Book/19-advanced-features/19.1-不安全Rust.md) · Nomicon [07 Send/Sync](../../04-Rust-Nomicon/07_Concurrency_Atomic/02-send-sync.md) · ER [Item 16](../../01-ER/Chapter-03-Concepts/Item-16-avoid-unsafe/README.md) · Async [Pin](../Chapter-08-Asynchronous-Programming/06-pin-unpin.md)

---

若**仅通过安全代码**误用 trait 实现即可内存不安全 → 必须 **`unsafe trait`** + **`unsafe impl`**。

```text
unsafe trait   →  声明：实现本 trait 有编译器无法检查的契约
unsafe impl    →  担保：我的类型遵守该契约，否则安全代码会 UB
```

---

## 一、两种 trait 安全边界

| | **普通 trait（安全 trait）** | **unsafe trait** |
|---|------------------------------|------------------|
| 编译器 | 可校验方法前置不变量 | **无法**验证 trait 约定的内存/并发不变量 |
| 调用 | 安全代码随便调用 | 安全代码可调用，但 **impl 须 `unsafe`** |
| `impl` | 普通 `impl` | **必须** `unsafe impl` |
| 误用后果 | 通常编译失败 | 错误 impl → **安全代码直接 UB** |

### 语法成对约束

```rust
// 定义
unsafe trait DangerousMarker {}

// 实现 — 不能省略 unsafe
unsafe impl DangerousMarker for MyType {}
```

### 语义本质

- **`unsafe trait T`**：向使用者宣告「实现本 trait 有编译器无法检查的安全契约」。
- **`unsafe impl T for X`**：程序员手动担保「`X` 完全遵守 `T` 的所有不变量」— 风险由**实现者**承担。

### 五大 Unsafe 超能力之四

实现 `unsafe trait` 属于 [01 §二](./01-unsafe-keyword.md) 五类仅能在 `unsafe` 语境执行的操作之一（声明/实现本身在类型系统层，不等同于写 `unsafe {}` 块，但 **impl 关键字必须是 `unsafe impl`**）。

---

## 二、何时应标记为 `unsafe trait`

满足**任意一条**，就应使用 `unsafe trait`：

| # | 判定 |
|:-:|------|
| 1 | 仅通过**安全代码**调用该 trait，错误 impl 就可能破坏内存不变量 |
| 2 | **Marker trait**（无方法）：类型具备编译器无法验证的属性，错误 impl 直接 UB（`Send`/`Sync`） |
| 3 | trait 方法有前置安全契约，编译器无法静态校验，必须由实现者保证 |

---

## 三、标准库核心：`Send` / `Sync`（unsafe marker）

### 1. 定义（空 trait，无方法）

```rust
// 概念上（标准库为 unsafe auto trait）
pub unsafe auto trait Send {}
pub unsafe auto trait Sync {}
```

| Trait | 契约 |
|-------|------|
| **`Send`** | 类型的**所有权**可安全**转移**到另一线程 |
| **`Sync`** | **`&T`** 可安全在多线程间**共享**（等价 `T: Sync` ⟺ `&T: Send`） |

→ Ch02 详解：[09.3 Send/Sync/Unpin](../Chapter-02-Types/09-3-send-sync-unpin.md)

### 2. 为什么是 `unsafe trait`？

**错误 `impl Send`/`Sync` 时，仅安全代码就能产生数据竞争（UB）**，编译器拦不住 impl 是否正确。

| 风险场景 | 后果 |
|----------|------|
| 裸指针 `*mut T` 手动 `Send`，多线程同时读写指向内存 | 数据竞争 |
| `Rc<T>` 强行 `Send`，跨线程并发改 refcount | UB |
| 无锁 `UnsafeCell` 手动 `Sync`，多线程并发写 | 破坏内存安全 |

### 3. 自动推导（auto trait）

编译器扫描**所有字段**：

- 成员均 `Send` → 类型自动 `Send`
- 成员均 `Sync` → 类型自动 `Sync`

含裸指针、`Rc`、`RefCell`、`UnsafeCell`、线程局部句柄等 → **取消**自动派生；若仍要跨线程，须手动 `unsafe impl` 并**完整审计**。

### 4. 手动实现模板

```rust
use std::sync::Mutex;

struct SafeBuffer {
    inner: Mutex<Vec<u8>>, // 内部同步，可论证 Send+Sync
}

// 若仅裸指针 + 自建同步：
struct RawBuffer(*mut u8);

unsafe impl Send for RawBuffer {}
unsafe impl Sync for RawBuffer {}
// 须在 // SAFETY: 注释中证明：所有访问经 Mutex/原子/单线程约束等
```

| 要点 | 说明 |
|------|------|
| 孤儿规则 | 仍适用 → [Ch02 §07.1](../Chapter-02-Types/07-1-orphan-rule.md) |
| 文档 | `unsafe impl` 前写清线程安全论证 → [11 文档](./11-documentation.md) |

---

## 四、`Unpin`：安全 trait，勿与 unsafe trait 混淆

### 1. `Unpin` 不是 `unsafe trait`

```rust
pub auto trait Unpin {} // 无 unsafe 修饰
```

`impl Unpin` 用**普通** `impl`，不需要 `unsafe impl`。

### 2. 危险来自 `Pin` 相关 **unsafe API**

| 概念 | 说明 |
|------|------|
| **`!Unpin`** | 内存固定后不能随意移动 — 自引用 `Future` 等 |
| **`Pin::new_unchecked`** | **unsafe fn** — 调用方保证不会移动被钉住对象 |
| 误 impl `Unpin` | 安全代码里移动 `Pin` 后可能破坏自引用 → UB；但**不是因为 `unsafe impl Unpin`** |

→ 深入：[第 8 章 Pin & Async](../Chapter-08-Asynchronous-Programming/06-pin-unpin.md)

---

## 五、易错区分表

| 特征 | `unsafe trait`（Send/Sync） | 普通 trait（Unpin） | `unsafe fn` |
|------|----------------------------|---------------------|-------------|
| 声明 | `unsafe trait T` | `trait T` | `unsafe fn f()` |
| impl | **必须** `unsafe impl` | 普通 `impl` | 无关 |
| 风险主体 | **实现者** | 误用 `Pin::new_unchecked` 等 | **调用者** |
| 安全代码危害 | 错误 impl → 安全代码 UB | 仅误用 unsafe API 才 UB | 安全代码**不能**直接调用 |

---

## 六、核心总结（背诵考点）

1. **`unsafe trait` 判定**：错误实现会让**安全代码**产生 UB。  
2. **语法成对**：`unsafe trait` ↔ `unsafe impl`，缺一不可。  
3. **`Send`/`Sync`**：最典型的 unsafe marker；auto 推导，特殊场景手动 impl 须完整审计线程安全。  
4. **`Unpin`**：安全 trait；风险在 `Pin` 的 unsafe API，不要和 unsafe trait 混为一谈。  
5. 实现 unsafe trait = 五大 unsafe 特权之一；**实现者**全权承担内存安全责任。

---

## 对照阅读

- Ch02 → [09.4 自动推导与 unsafe impl](../Chapter-02-Types/09-4-unsafe-impl.md)
- Book → [16.4 Send/Sync](../../00-Book/16-fearless-concurrency/16.4-Send与Sync.md) · [19.1 unsafe](../../00-Book/19-advanced-features/19.1-不安全Rust.md)
- Nomicon → [07 Send & Sync](../../04-Rust-Nomicon/07_Concurrency_Atomic/02-send-sync.md)
- ER → [Item 16 避免 unsafe](../../01-ER/Chapter-03-Concepts/Item-16-avoid-unsafe/README.md)

---

## 速记

## 判定

错误 impl → **安全代码 UB** → 用 `unsafe trait` + `unsafe impl`。

## 语法

```rust
unsafe trait DangerousMarker {}
unsafe impl DangerousMarker for MyType {}
```

## Send / Sync

- **unsafe auto trait**；空 marker  
- 错 impl → 数据竞争 UB  
- 字段均满足 → 自动推导；否则手动 `unsafe impl` + 审计  

## Unpin ≠ unsafe trait

- `trait Unpin` — 普通 `impl`  
- 风险在 `Pin::new_unchecked`（unsafe fn）

## 对照

| | unsafe trait | unsafe fn |
|---|--------------|-----------|
| 谁担责 | **实现者** | **调用者** |
| 例子 | `Send`/`Sync` | `assume_init` |

## 自测

- [ ] 为何 `Send` 是 unsafe trait 而不是普通 trait？  
- [ ] `Unpin` 为何不需要 `unsafe impl`？  
- [ ] 五大 unsafe 超能力里哪一条对应 `unsafe impl Send`？

