# 第 11 章 · 并发 —— 动手 Lab

> 上：[chapter11_concurrency](../README.md) · 笔记：[11.1 Futex](../11.1-futex.md) · [11.2 Mutex](../11.2-mutex-overview.md) · [11.6 Once](../11.6-once.md) · [11.10 MPSC](../11.10-mpsc-overview.md)

## 跑起来

```bash
cargo test --manifest-path 17-rust-foundation/03-DeepRustStdLib/chapter11_concurrency/demo/concurrency_lab/Cargo.toml
```

12 个测试，零 error 零 warning。

## 实现了什么

| 类型 | 对应 std | 看点 |
|---|---|---|
| `SpscRing<T>` | （std 没有，HFT 自己造） | 无锁 SPSC 环形队列 |
| `MiniMutex<T>` | `Mutex<T>` | CAS 状态机 + `UnsafeCell` + RAII guard |
| `MiniOnce` | `Once` | 三态：未完成 / 进行中 / 完成 |
| `MiniOnceLock<T>` | `OnceLock<T>` | `Once` + `UnsafeCell<MaybeUninit<T>>` |

## 一、为什么没有"真 futex"

futex 是 Linux 系统调用，Windows 对应 `WaitOnAddress`。std 把它们收在 `sys::sync`
（笔记里"三层架构"的最底层），上层 `std::sync` 完全不知道平台差异。

本 lab 要 Windows / macOS / Linux 都能跑，所以竞争路径用 **自旋 + 退让** 近似：

| | 真实 std | 本 lab |
|---|---|---|
| 无竞争 | 一条 CAS，**零 syscall** | 一条 CAS |
| 有竞争 | `futex(FUTEX_WAIT)` 睡眠 | 先 `spin_loop` 64 次，再 `yield_now` |

**快路径语义完全一致**，慢路径只是"不睡觉"——对理解状态机没有损失。

## 二、`SpscRing`：HFT 里最常用的一块砖

### 三处关键设计

**1. 容量必须是 2 的幂** —— 用 `idx & (cap - 1)` 代替 `idx % cap`，
省一次除法。在每秒百万次的行情通路里这是实打实的收益。

**2. cacheline 隔离** —— `head`（消费者写）和 `tail`（生产者写）各占一条 cacheline：

```rust
#[repr(align(64))]
struct CachePadded<T> { value: T }
```

否则两侧 CPU 会互相把对方的 line 置为 Invalid（**伪共享**），
无锁队列的性能优势会被吃干净。

**3. 四组内存序，一个都不能错：**

| 位置 | 顺序 | 为什么 |
|---|---|---|
| 生产者读 `tail` | `Relaxed` | 只有自己写，无需同步 |
| 生产者读 `head` | `Acquire` | 要看到消费者"释放"出来的槽位 |
| 生产者写 `tail` | `Release` | 把刚写入的元素发布给消费者 |
| 消费者侧 | 对称 | `head` 的读写反过来 |

### 并发契约

`push` / `pop` 用 `&self`，但要求**同一时刻只有一个生产者线程、一个消费者线程**。
这正是 SPSC 的前提，也是它敢用 `Relaxed` 的原因。契约破了就是 UB。

### 析构

`Drop` 里先把没取走的元素 `pop` 出来析构，再 `dealloc` 裸内存 ——
否则残留元素会被泄漏（测试 `spsc_drop_drops_leftover_items` 钉住这条）。

## 三、`MiniMutex`：锁就是 `UnsafeCell` + 状态机

```rust
pub struct MiniMutex<T> {
    locked: AtomicBool,     // 状态机
    data: UnsafeCell<T>,    // 第 7 章的主角
}
```

- `lock()` 快路径一条 `compare_exchange(..., Acquire, Relaxed)`；
- guard 的 `Drop` 用 `Release` 把临界区内的所有写发布出去；
- `unsafe impl<T: Send> Sync for MiniMutex<T>` —— **这就是 `Mutex<T>: Sync` 的由来**，
  也是 `Arc<Mutex<T>>` 能跨线程共享的根因。

### guard 是 `!Send`

std 用 `negative impl` 声明 `MutexGuard: !Send`（稳定版拿不到）。
这里用一个小技巧达到同样效果：

```rust
_no_send: PhantomData<std::rc::Rc<()>>,   // Rc 不是 Send ⇒ guard 自动 !Send
```

含义：**不能在线程 A 加锁、把 guard 送到线程 B 再解锁**。

## 四、`MiniOnce`：三态状态机

```text
INCOMPLETE --CAS--> RUNNING --store--> COMPLETE
     |                  ^
     └──── 失败者：自旋等到 COMPLETE ────┘
```

`Acquire` 看到 `COMPLETE` ⇒ 一定能看到初始化闭包里的所有写（配对的 `Release`）。

⚠️ **本实现没有中毒（poison）处理**：若闭包 panic，状态停在 `RUNNING`，
后续等待者会一直自旋。std 的做法是标记"中毒"并让后续调用 panic。

## 五、测试清单

| 测试 | 验证点 |
|---|---|
| `spsc_preserves_fifo_order` | FIFO 顺序 |
| `spsc_full_returns_the_value_back` | 满队列时值被**退回**而非丢弃 |
| `spsc_empty_returns_none` | 空队列 |
| `spsc_capacity_must_be_power_of_two` | 2 的幂断言 |
| `spsc_drop_drops_leftover_items` | 析构不泄漏 |
| `spsc_cross_thread_transfers_everything` | **真并发**：100 万元素跨线程传输并校验和 |
| `mutex_counter_from_many_threads` | 8 线程 × 1 万次自增 = 8 万，无丢失 |
| `mutex_try_lock_fails_while_held` | `try_lock` 语义 |
| `mutex_into_inner_and_deref_mut` | `DerefMut` |
| `once_runs_exactly_once_under_many_threads` | 16 线程争抢，闭包只跑 1 次 |
| `once_lock_get_or_init_is_idempotent` | `get_or_init` 幂等 |
| `once_lock_set_twice_second_fails` | 第二次 `set` 原值退回 |

## 与 HFT 的关系

`SpscRing` 是行情网关里最常见的一块砖：
**网卡中断线程 / 解码线程（生产者）→ 策略线程（消费者）** 之间，
要么用无锁 SPSC 队列，要么用 ring buffer + 忙轮询。
它的两个性能命门（掩码取模、cacheline 隔离）在真实的低延迟系统里同样是命门。
