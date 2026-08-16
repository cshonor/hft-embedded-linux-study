# 原子类型与内存排序 — 第 2～3 章贯通笔记

> 原子操作是 **Mutex、Condvar** 等高级原语的基石。本文综合《Rust Atomics and Locks》**第 2 章（原子 API）** 与 **第 3 章（内存排序）**，便于考试/面试一次背全。  
> 代码：`Chapter-02-Atomics/`（API）、`use_atomic.rs` 中 **Release/Acquire**、`use_fence.rs`、`use_seqcst.rs`。

---

## 1. 普通变量与原子变量的核心差异

多线程下有三条本质区别：

| 维度 | 普通变量 | 原子变量 |
|------|----------|----------|
| **不可分割 / 数据竞争** | 并发非原子读写 → **数据竞争** → **UB** | 读写在语言/硬件上**不可分割**（要么做完要么没做），避免该变量上的 DR |
| **内部可变性** | 修改需 **`&mut T`（排他借用）** | 多线程仅持 **`&Atomic*`** 也可安全改值（内部可变性） |
| **优化控制** | 编译器/CPU 默认「无他线程」，可重排、消除读写 | 每次操作须传 **`Ordering`**，约束重排与可见性 |

**Atomic（ἄτομος）**：不可分割 — 高级并发原语都建立在此之上。

---

## 2. 基础原子变量类型

Rust **没有**通用 `Atomic<T>`：需硬件指令支持，可用类型因平台而异；一般至少支持到**指针宽度**：

| 类别 | 类型 |
|------|------|
| 布尔 | `AtomicBool` |
| 整型 | `AtomicI8` … `AtomicI64`、`AtomicIsize` / `AtomicU8` … `AtomicUsize` |
| 指针 | `AtomicPtr<T>` |

---

## 3. 原子操作基础 API 与场景

三类 API（所有原子类型接口一致）：

### A. Load / Store

| API | 场景 |
|-----|------|
| `load(Ordering)` / `store(val, Ordering)` | **Stop Flag**：`AtomicBool`，主线程 `store(true)`，后台 `load` 循环退出 |
| | **进度条**：后台 `store` 进度 `AtomicUsize`，主线程 `load` 展示 |

→ 代码：`use_atomic.rs`（`demo_atomic_bool_stop_flag` 等）

### B. Fetch-and-Modify

单次指令：**读旧值 + 写新值**，返回**操作前**旧值。

| API 族 | 场景 |
|--------|------|
| `fetch_add`、`fetch_sub`、`fetch_or`、`fetch_max`… | **多线程计数**：`fetch_add(1)`，禁止多线程互 `store` 覆盖 |
| `swap` | 直接替换并返回旧值 |
| `fetch_add(1)` | **唯一 ID** 分配 |

→ 代码：`use_atomic_operations.rs`、`id_allocator.rs`

### C. Compare-and-Exchange（CAS）

若当前值 == **期望值** → 写入**新值** 并 `Ok(期望)`；否则不改，返回 `Err(当前值)`。

| API | 场景 |
|-----|------|
| `compare_exchange` / `compare_exchange_weak` | **一次性懒加载**：仅 `CAS(0, new_key)` 成功者写入；`Err` 则丢弃自己的 key，用已有值 |

→ 代码：`lazy_init.rs`、`id_allocator.rs`（防溢出）

> 第 2 章案例多用 **`Relaxed`**：只保证**该原子变量**上的原子性与总修改顺序，**不**保证与其他内存的 happens-before（见 §4）。

---

## 4. 内存排序（`Ordering`）— 第 3 章核心

「不可分割」不够；跨线程通信还要约束**指令重排与可见性**。每次 `load`/`store`/`fetch_*`/`compare_exchange` 都要传 **`std::sync::atomic::Ordering`**。

### 4.1 `Relaxed`（宽松）

- **语义**：**无**跨线程 happens-before；只保证**同一原子变量**上修改的**全局一致顺序**（总修改顺序）。
- **适用**：变量**独立**、不同步其他内存（纯计数、进度、Stop Flag 仅控制自身循环时等）。
- **禁止**：原子是「数据已就绪」的**信使**，却用 Relaxed → 他线程可能看到标志已变、**普通数据仍旧**。

### 4.2 `Release` / `Acquire`（释放 / 获取）

- **语义**：成对使用。A  **`Release` store** 后，B **`Acquire` load** 读到该写入 → 建立 **happens-before**。
- **效果**：A 在 Release **之前**对**任意内存**的写入，在 B Acquire **之后**对 B **可见**。
- **典型**：`Mutex` 解锁 ≈ Release，加锁 ≈ Acquire，保护锁内数据。

→ 代码：`use_atomic.rs`（`demo_release_acquire` 等）

### 4.3 `AcqRel`（获取并释放）

- **语义**：用于 **RMW**；读侧 Acquire + 写侧 Release。
- **适用**：`fetch_*`、`compare_exchange`、`swap` 等同时充当同步点。

### 4.4 `SeqCst`（顺序一致）

- **语义**：最强；含 Release/Acquire 效果，且所有带 **SeqCst** 的操作在所有线程眼中共有一条**全局严格顺序**。
- **适用**：**极少**（如双旗帜：需两个不同原子的全局写入先后）。
- **审查信号**：滥用 SeqCst 常意味着未分析清需求 — 多数场景 **Acquire/Release** 或 **Relaxed** 即可。

→ 代码：`use_seqcst.rs`、`use_fence.rs`

### 4.5 对照表（必背）

| Ordering | 跨变量顺序 | 典型用途 |
|----------|------------|----------|
| **Relaxed** | 无 HB | 独立计数、进度 |
| **Release** | 与配对 Acquire 建立 HB | 发布 / 解锁 |
| **Acquire** | 读到 Release 后见之前写 | 订阅 / 加锁 |
| **AcqRel** | RMW 双向 | fetch/CAS/swap |
| **SeqCst** | 全局 SeqCst 顺序 | 多原子全局时间线 |

### 4.6 第 4 章：自旋锁实战

`swap(true, Acquire)` 加锁、`store(false, Release)` 解锁 → [Chapter-04-Spin-Locks/本章学习笔记.md](./Chapter-04-Spin-Locks/本章学习笔记.md)

---

## 5. 使用规范与易错点

| # | 易错点 | 规范 |
|---|--------|------|
| 1 | **`fetch_add` 静默回绕** | ID/索引有上限 → **`loop` + `compare_exchange_weak`**，边界与更新一体；勿在 `fetch_add` 后再 `panic`（已发生） |
| 2 | **CAS 循环** | 在 `loop` 里优先 **`compare_exchange_weak`**（ARM 等可能伪失败，重试即可） |
| 3 | **滥用 SeqCst** | 先问能否 **Acquire/Release** 或 **Relaxed**；SeqCst 需有明确全局顺序需求 |
| 4 | **Relaxed 当信使** | 标志位通知「别内存已写好」→ 必须 **Release store + Acquire load** |

---

## 6. 面试一句话

**原子变量用不可分割的 load/store/fetch/CAS 避免 DR，并用 Ordering 约束可见性与顺序：Relaxed 只保单变量顺序；Release/Acquire 同步其他内存；SeqCst 最强慎用；fetch 会回绕，关键路径用 CAS 循环。**

---

## 7. 极简背诵卡

| # | 背这句 |
|---|--------|
| 1 | 普通变量并发非原子写→UB；原子不可分割+内部可变+须传 Ordering。 |
| 2 | 无 Atomic&lt;T&gt;；Bool/整型/AtomicPtr，平台支持到指针宽。 |
| 3 | load/store 停标志与进度；fetch 多线程计数与 ID；CAS 一次初始化。 |
| 4 | Relaxed 无跨线程 HB，只保该原子总顺序；不当「数据就绪」信使。 |
| 5 | Release 写+Acquire 读成对→HB；Release 前写对 Acquire 后读者可见。 |
| 6 | SeqCst 全局最严顺序；能不用就不用，复杂或偷懒信号。 |
| 7 | fetch 溢出回绕；loop+compare_exchange_weak；CAS 循环用 weak。 |

---

## 8. 分章深入

| 章节 | 文件 |
|------|------|
| 第 2 章 API 与案例 | [Chapter-02-Atomics/本章学习笔记.md](./Chapter-02-Atomics/本章学习笔记.md) |
| 第 3 章内存序专题 | [Chapter-03-Memory-Ordering/本章学习笔记.md](./Chapter-03-Memory-Ordering/本章学习笔记.md) |
| 第 4 章自旋锁实战 | [Chapter-04-Spin-Locks/本章学习笔记.md](./Chapter-04-Spin-Locks/本章学习笔记.md) |
