# 读写锁（RwLock）— 跨章贯通笔记

> **书目澄清**：《Rust Atomics and Locks》**第 7 章是「理解处理器」**（x86/ARM、缓存一致性、内存屏障等），**不是** RwLock 专章。  
> **RwLock 原理、写者饥饿、手写写优先实现** 集中在：**第 1 章**（`std::sync::RwLock` 概念）与 **第 9 章**（自建防饥饿 RwLock）。  
> 与 Mutex 对照：[互斥锁与锁体系-贯通笔记.md](./互斥锁与锁体系-贯通笔记.md)

---

## 全书章节与 RwLock 的对应

| 章 | 文件夹 | 内容 |
|----|--------|------|
| 1 | `Chapter-01-Rust-Concurrency-Basics` | `RwLock<T>` 用法、与 `RefCell` 类比 |
| 7 | `Chapter-07-Processors` | **缓存行、假共享**（影响锁性能，非 RwLock API） |
| 9 | `Chapter-09-Custom-Locks` | [9.3.2 写优先 RwLock](./Chapter-09-Custom-Locks/9.3-reader-writer-lock/9.3.2-writer-starvation-state-machine.md) |

---

## 1. 核心原理与读写分离优势

`std::sync::RwLock<T>` ≈ 多线程版 **`RefCell<T>`**：运行期追踪借用，遵守 **共享 `&T` 或 独占 `&mut T`**。

### 三态模型（概念）

| 状态 | 含义 |
|------|------|
| **未锁定** | 无读者、无写者 |
| **读者锁定** | 任意多个读者，共享 `&T` |
| **写者锁定** | 唯一写者，独占 `&mut T` |

`Mutex` 只有两态，且**读也要独占** → 纯读场景也会互相阻塞。

### 读写分离的优势

- **读多写少**：多线程可同时 `read()`，吞吐量远高于「每次读都抢 Mutex」。  
- **典型业务**：配置/传感器数据偶尔写、大量工作线程高频读（书中例子）。

---

## 2. 读优先 vs 写优先与写者饥饿

### 读者优先 → 写者饥饿（Writer Starvation）

- 简单实现（如部分 **pthread_rwlock**）：已有读者时，新读者常**直接加入**。  
- 若读者**源源不断**，读计数永不为 0 → **写者永远拿不到锁**。

### 写者优先（书中第 9 章方案）

- **一旦有写者排队，拦截所有新读者**（新读者也休眠）。  
- 现有读者执行完 `Drop` 后计数归零 → 写者获得锁。  
- 保证写者**最终**能执行，且后续读者能看到**最新**数据。

---

## 3. `Mutex` vs `RwLock` 选型

| | `Mutex<T>` | `RwLock<T>` |
|---|------------|-------------|
| **内部** | 简单两态 | 读者计数 + 双队列唤醒，更复杂 |
| **临界区极短** | 常更快（状态机开销可超过 `x += 1`） | 可能更慢 |
| **读多写少** | 读者互斥，浪费 | **首选** |
| **读写差不多** | **首选** | 过度设计 |
| **解锁唤醒** | 常 `wake_one` | 写者释放后常需 **`wake_all` 读者** → 惊群成本 |

**误区**：RwLock 并非默认优于 Mutex；按访问模式选型。

### 平台陷阱（第 1 章提及）

- **macOS pthread**：部分实现偏公平，性能特点不同。  
- **Windows SRWLock**：读写排队；**已持读锁时再申请读锁且写者排队** → 可能**永久死锁**。Rust 标准库实现会规避部分 POSIX 问题，但**逻辑上仍须避免**「读锁内再读锁且可能有写者等待」的反模式。

---

## 4. 第 9 章：写优先 RwLock 状态机（核心思路）

书中用 **`AtomicU32 state`**（配合 `atomic_wait` 的 `wait` / `wake_one` / `wake_all`）：

| 编码技巧 | 含义 |
|----------|------|
| **读者数 × 2** 存入 `state` | 用 **偶/奇** 区分是否有写者排队 |
| **偶数** | 无写者排队 → **新读者可加入**（`state += 2`） |
| **奇数** | 有写者排队 → **新读者必须 wait** |
| **`u32::MAX`** | 写者已持有写锁 |

### 写者 `write()` 三步

1. 若可独占（`s <= 1`）→ CAS 为 `MAX`。  
2. 若当前为**偶数**且有读者 → **`state += 1` 变奇数**，举起「写者等待」旗，**拦截新读者**。  
3. 在 `writer_wake_counter` 上 **wait**，由**最后一个读者** `Drop` 时唤醒。

### 读者 `read()` 两步

1. 仅当 **`s % 2 == 0`** 时 CAS `s + 2` 加入。  
2. 若 **`s % 2 == 1`** 或已是 `MAX` → **wait** 在 `state` 上。

完整代码骨架见：**[9.3.2-writer-starvation-state-machine.md](./Chapter-09-Custom-Locks/9.3-reader-writer-lock/9.3.2-writer-starvation-state-machine.md)**（依赖 `atomic_wait`，与书一致）。

---

## 5. 与第 7 章（处理器）的关系

- **假共享**：多个 `RwLock` 挤在同一缓存行 → 性能暴跌 → `#[repr(align(64))]`（见 Mutex 贯通笔记 §5.4）。  
- **内存屏障**：`Acquire`/`Release` 在 `read`/`write` CAS 上的用法与第 3 章一致。  
- 详见 **`Chapter-07-Processors`**（待补汇编/缓存笔记）。

---

## 6. 标准库用法（第 1 章）

```rust
use std::sync::RwLock;

let lock = RwLock::new(vec![1, 2, 3]);
{
    let r = lock.read().unwrap();
    println!("{:?}", *r);
} // ReadGuard drop → 释放读锁
{
    let mut w = lock.write().unwrap();
    w.push(4);
}
```

- `read()` / `write()` 返回 `Result`（**中毒**同 Mutex）。  
- Guard 实现 `Deref` / `DerefMut`，RAII 释放。

---

## 7. 面试一句话

**RwLock 允许多读或单写，适合读多写少；简单实现会写者饥饿，第 9 章用 state 奇偶拦截新读者实现写优先；临界区很短时 Mutex 可能更快；注意 SRW 读锁递归排队死锁。**

---

## 8. 极简背诵卡

| # | 背这句 |
|---|--------|
| 0 | 第 7 章是处理器/缓存，不是 RwLock；RwLock 在 1、9 章。 |
| 1 | RwLock≈多线程 RefCell；三态：无锁/多读/单写。 |
| 2 | 读多写少吞吐高；Mutex 读也互斥。 |
| 3 | 读者优先→写者饥饿；写者优先→拦截新读者。 |
| 4 | 临界区极短用 Mutex；RwLock 状态机更重。 |
| 5 | 解锁读者常 wake_all；惊群成本。 |
| 6 | state 读者数×2；偶=无写者排队，奇=写者排队。 |
| 7 | 写者偶数时 state+1 拦新读者；最后读者唤醒写者。 |
