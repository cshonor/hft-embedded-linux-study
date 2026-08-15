# 条件变量（Condvar）— 跨章贯通笔记

> **书目澄清**：《Rust Atomics and Locks》**第 8 章 = 操作系统同步原语**（futex、`pthread_cond`、Windows `CONDITION_VARIABLE` / `WaitOnAddress` 等），是 Condvar/Mutex **阻塞与唤醒的底层**。  
> **Condvar 用法与生产者-消费者** → **第 1 章**；**虚假唤醒、惊群、持锁 notify 等** → **第 1 章 + 第 9 章**。  
> 配套代码：**`Chapter-01-Rust-Concurrency-Basics/use_condvar.rs`**

---

## 全书章节与 Condvar 的对应

| 章 | 文件夹 | 内容 |
|----|--------|------|
| 1 | `Chapter-01-Rust-Concurrency-Basics` | `Condvar` + `Mutex`、**while 循环 wait**、生产者-消费者 |
| 8 | `Chapter-08-OS-Primitives` | **futex WAIT/WAKE/REQUEUE**、`pthread_cond_*`、Windows 原语 |
| 9 | `Chapter-09-Custom-Locks` | 惊群、`notify_one` vs `notify_all`、与自建锁 wait 队列 |

---

## 1. Condvar 做什么

**在条件不满足时让线程高效休眠；条件满足时被唤醒** — 避免无意义 **busy-loop** 烧 CPU。

- **必须与 `Mutex` 配合**：等待的是「在持锁下检查仍不满足的条件」。  
- **解决竞态**：若先解锁再 sleep，可能 **漏掉** 中间发生的 `notify`；`wait(guard)` 在底层 **原子地「解锁 + 入队休眠」**。

---

## 2. 底层 OS 原语（第 8 章）

| 平台 | 机制 | 要点 |
|------|------|------|
| **Linux** | **futex** | `FUTEX_WAIT`：原子变量仍为期望值则休眠；`FUTEX_WAKE` 唤醒；`FUTEX_REQUEUE` 可把等 Condvar 的线程**重排队**到等 Mutex，减轻惊群 |
| **POSIX / macOS** | **`pthread_cond_t`** + **`pthread_mutex_t`** | `pthread_cond_wait` / `timedwait`；`signal`（≈one）/ `broadcast`（≈all） |
| **Windows** | **`CONDITION_VARIABLE`** | 与 SRWLock / Critical Section；或 **`WaitOnAddress`** + **`WakeByAddressSingle/All`** |

Rust `std::sync::Condvar` 封装上述实现，日常不直接调 syscall。

---

## 3. Wait / Notify 完整流程

### 3.1 `wait(guard)` 的原子性

1. **原子「解锁 + 休眠」**：传入的 **`MutexGuard` 在 wait 内部被消费**；线程进入等待队列时**已不持锁**。  
2. **被唤醒后**：可能因 **notify** 或 **虚假唤醒**；在返回前 **重新加锁**，得到**新的** `MutexGuard`。  
3. **返回 `Result`**：若 Mutex **中毒**（持锁线程 panic），得 `Err`。

### 3.2 标准写法（Must）

**禁止** 单独 `if` + 一次 `wait`；**必须** `while` / `loop` 检查条件：

```rust
let mut guard = queue.lock().unwrap();
while guard.is_empty() {
    guard = not_empty.wait(guard).unwrap();
}
let item = guard.pop_front().unwrap();
// 处理 item 前可 drop(guard) 缩小临界区
```

### 3.3 `notify_one` vs `notify_all`

| API | 行为 | 适用 |
|-----|------|------|
| **`notify_one()`** | 唤醒**一个**等待者 | 每轮只产生**一份**工作（一个槽位、一条消息） |
| **`notify_all()`** | 唤醒**全部** | 状态变化影响**所有**等待者（广播式）；慎用，易惊群 |

---

## 4. 生产者-消费者模型（第 1 章）

**共享状态**（典型）：

```rust
let queue = Mutex::new(VecDeque::new());
let not_empty = Condvar::new();
// 常包在 Arc<struct { queue, condvar }> 里多线程共享
```

| 角色 | 逻辑 |
|------|------|
| **消费者** | `lock` → `while empty { wait }` → `pop` → **`drop(guard)`** 再处理（避免持锁处理耗时） |
| **生产者** | `lock` → `push` → **锁随作用域结束释放** → **`notify_one()`** |

→ 完整示例：**`use_condvar.rs`**（`demo_condvar_basic` 等）

与 **第 5 章 One-Shot**（`park`/`unpark`）对比：Condvar 表达「**条件**」；park 只适合简单「有人叫我」语义。

---

## 5. 常见错误与排查（第 1 + 9 章）

### 错误一：虚假唤醒（Spurious Wake-up）

| 项 | 内容 |
|----|------|
| **现象** | 无人 `notify`，线程仍从 `wait` 返回 |
| **后果** | `if empty { wait }` 一次醒来后以为非空 → **空队列 pop panic** |
| **解决** | **`while !condition { wait }`**，醒来**先再检查** |

### 错误二：惊群（Thundering Herd）

| 项 | 内容 |
|----|------|
| **现象** | `notify_all` 唤醒大量线程 → 全部抢**同一把 Mutex** → 仅 1 个成功，其余再睡 |
| **后果** | CPU 与上下文切换飙升，吞吐差 |
| **解决** | 单份产出用 **`notify_one`**；高级场景用 **futex REQUEUE**（第 8/9 章） |

### 错误三：一个 Condvar 绑多把 Mutex

| 项 | 内容 |
|----|------|
| **现象** | A 用 `mutex1.wait(cond)`，B 用 `mutex2.wait(cond)` |
| **后果** | 未定义行为或 **panic**（实现相关） |
| **解决** | **一个 Condvar 只配合一把 Mutex**（及它保护的条件） |

### 错误四：持锁 `notify` 导致二次阻塞

| 项 | 内容 |
|----|------|
| **现象** | 生产者在**仍持锁**时 `notify`；消费者醒来要在 `wait` 里**重新加锁**，锁还被生产者占着 → **再睡一次** |
| **安全** | 一般不破坏内存安全 |
| **性能** | 多一次调度；**推荐**：`push` 后 **drop(guard)**，**再** `notify_one()`（书中生产者即：`lock` 作用域结束释放锁，再 notify） |

### 与 `if let` 延长锁（Mutex 专题）

`if let Some(x) = queue.lock().unwrap().pop()` 会让 **Guard 活到整个 if 结束** — 对 Condvar 路径同样适用：**先 `let` 再判断**。

---

## 6. 与其它机制对照

| 机制 | 等待什么 | 典型场景 |
|------|----------|----------|
| **Condvar** | **条件**（队列非空等） | 生产者-消费者、线程池任务队列 |
| **thread::park** | 无结构化条件 | 简单 handoff（`thread_advanced`） |
| **Mutex** | 锁可用 | 独占数据 |
| **自旋锁** | 锁可用（忙等） | 极短临界区 |

---

## 7. 面试一句话

**Condvar 与 Mutex 配合：wait 原子解锁并休眠，醒来须 while 重检条件防虚假唤醒；生产者先释放锁再 notify_one 防二次阻塞；notify_all 易惊群；底层 Linux 用 futex，POSIX 用 pthread_cond。**

---

## 8. 极简背诵卡

| # | 背这句 |
|---|--------|
| 0 | 第 8 章 OS 原语；Condvar 用法在第 1 章，排错在第 9 章。 |
| 1 | Condvar=条件不满足时休眠，满足时唤醒；必配 Mutex。 |
| 2 | wait 原子解锁+休眠；返回前重新加锁得新 Guard。 |
| 3 | 必须 while/loop 里 wait，防虚假唤醒。 |
| 4 | 生产者 push 后放锁再 notify_one；消费者 while 空则 wait。 |
| 5 | notify_all 惊群：全醒抢一把锁；单产出用 notify_one。 |
| 6 | 一个 Condvar 只绑一把 Mutex。 |
| 7 | Linux futex WAIT/WAKE；REQUEUE 减惊群。 |
