#!/usr/bin/env python3
from pathlib import Path

CH2 = Path(__file__).resolve().parents[1] / "Chapter-02-Atomics"
M = "05-Async-Concurrency-Network/01-atomic/Cargo.toml"


def w(rel: str, text: str) -> None:
    p = CH2 / rel
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(text.strip() + "\n", encoding="utf-8")


w(
    "2.1-atomic-load-store/2.1-atomic-load-store.md",
    f"""## 2.1 Atomic Load and Store（原子加载与存储）

> 书 §2.1 · 贯通：[Atomics与内存序-贯通笔记.md](../../Atomics与内存序-贯通笔记.md) · 内存序精读见 [Chapter-03](../Chapter-03-Memory-Ordering/本章学习笔记.md)  
> **本节正文已拆分**；按 **2.1.1 → 2.1.4** 顺序阅读。

---

## 读 §2.1 前：数据竞争与原子性（跨章底座）

**数据竞争（Data Race）**：一线程写、另一线程同时读写同一块内存 → **UB**。Rust 默认用借用排除；跨线程无锁共享**单个变量**时用 **`Atomic*`**，保证该变量上的读/写不可分割。

| 层级 | 行为 |
|------|------|
| 编译器 | 可重排、删冗余读写（默认不知其他线程） |
| CPU / 缓存 | Store Buffer、乱序 → **可见性**无保证 |
| 原子操作 | 单变量 RMW 不可分割；**Relaxed** 只保证这一点，不保证多变量顺序（第 3 章） |

---

## §2.1 子笔记索引

| 子节 | 主题 | 笔记 |
|------|------|------|
| **2.1.1** | Stop Flag · `AtomicBool` | [2.1.1-stop-flag.md](./2.1.1-stop-flag.md) |
| **2.1.2** | 进度报告 · `AtomicI32` load/store | [2.1.2-progress-reporting.md](./2.1.2-progress-reporting.md) |
| **2.1.3** | Synchronization（与第 3 章衔接） | [2.1.3-synchronization.md](./2.1.3-synchronization.md) |
| **2.1.4** | 懒加载 · 竞争初始化 vs DR | [2.1.4-lazy-init.md](./2.1.4-lazy-init.md) |

**阅读顺序**：**2.1.1 → 2.1.2 → 2.1.3 → 2.1.4** → [2.2 索引](../2.2-fetch-and-modify/2.2-fetch-and-modify.md)

---

## Demo 目录

| 主题 | 文件 |
|------|------|
| Stop flag · 进度 | [2.1-atomic-load-store-demo.rs](./code/2.1-atomic-load-store-demo.rs) |
| 懒加载 · CAS 对比 | [2.1-atomic-load-store-lazy-init-demo.rs](./code/2.1-atomic-load-store-lazy-init-demo.rs) |

章索引：[本章学习笔记.md](../本章学习笔记.md)""",
)

w(
    "2.1-atomic-load-store/2.1.1-stop-flag.md",
    """## 2.1.1 Example: Stop Flag（停止标志）

> 书 §2.1 · 索引：[2.1](./2.1-atomic-load-store.md) · 后：[2.1.2 进度报告](./2.1.2-progress-reporting.md)

---

后台线程 `while !STOP.load(Relaxed)`，主线程 `STOP.store(true, Relaxed)` 通知退出。

- 用 **`static AtomicBool`** 或堆上 `Arc<AtomicBool>`（需 `'static` 时）
- 本章默认 **`Ordering::Relaxed`**：只保证 load/store 原子性，不建立跨变量顺序
- Demo：`demo_atomic_bool_stop_flag` → [code/2.1-atomic-load-store-demo.rs](./code/2.1-atomic-load-store-demo.rs)

§2.1 索引：[2.1-atomic-load-store.md](./2.1-atomic-load-store.md)""",
)

w(
    "2.1-atomic-load-store/2.1.2-progress-reporting.md",
    """## 2.1.2 Example: Progress Reporting（进度报告）

> 书 §2.1 · 索引：[2.1](./2.1-atomic-load-store.md) · 前：[2.1.1](./2.1.1-stop-flag.md) · 后：[2.1.3](./2.1.3-synchronization.md)

---

工作线程 `store` 进度百分比，主线程 `load` 打印。可配合 `thread::park_timeout` + `unpark` 降低轮询延迟。

- **单变量**进度用 load/store 即可
- **多线程各自 store 同一计数器会互相覆盖** → 见 [2.2.1 fetch_add](../2.2-fetch-and-modify/2.2.1-multi-thread-progress.md)

Demo：`demo_atomic_i32_progress` → [code/2.1-atomic-load-store-demo.rs](./code/2.1-atomic-load-store-demo.rs)""",
)

w(
    "2.1-atomic-load-store/2.1.3-synchronization.md",
    """## 2.1.3 Synchronization（同步）

> 书 §2.1 · 索引：[2.1](./2.1-atomic-load-store.md) · 后：[2.1.4 懒加载](./2.1.4-lazy-init.md)

---

**Relaxed** 的 load/store **不**保证：「线程 A 的 store 对线程 B 的后续 load **可见**」的通用顺序（仅保证单变量原子性）。

- Stop flag / 进度在书中先用 Relaxed 讲 API；**何时必须 Release/Acquire** → [Chapter-03 §3.3](../Chapter-03-Memory-Ordering/本章学习笔记.md)
- 贯通：[Atomics与内存序-贯通笔记.md](../../Atomics与内存序-贯通笔记.md)

§2.1 索引：[2.1-atomic-load-store.md](./2.1-atomic-load-store.md)""",
)

w(
    "2.1-atomic-load-store/2.1.4-lazy-init.md",
    """## 2.1.4 Example: Lazy Initialization（惰性初始化）

> 书 §2.1 · 索引：[2.1](./2.1-atomic-load-store.md) · CAS 版见 [2.3.2](../2.3-compare-and-exchange/2.3.2-lazy-one-time-init.md)

---

`AtomicUsize` 初值 0：`load` 为 0 则计算并 `store` 结果。多线程可能**重复计算**相同常量 → **竞争初始化（Race）**，不是 **Data Race**（无「非原子并发写普通内存」）。

- 若只需**幂等常量**，Relaxed store 可接受
- 若需**唯一随机密钥**、只初始化一次 → 必须用 **CAS**，见 [2.3.2](./2.3.2-lazy-one-time-init.md)

Demo：[code/2.1-atomic-load-store-lazy-init-demo.rs](./code/2.1-atomic-load-store-lazy-init-demo.rs)""",
)

w(
    "2.2-fetch-and-modify/2.2-fetch-and-modify.md",
    """## 2.2 Fetch-and-Modify Operations（读改写）

> 书 §2.2 · 专题：[CAS与Fetch-Modify专题.md](../CAS与Fetch-Modify专题.md)  
> **本节正文已拆分**；按 **2.2.1 → 2.2.3** 顺序阅读。

---

## §2.2 子笔记索引

| 子节 | 主题 | 笔记 |
|------|------|------|
| **2.2.1** | 多线程进度 · `fetch_add` | [2.2.1-multi-thread-progress.md](./2.2.1-multi-thread-progress.md) |
| **2.2.2** | 统计 total / max | [2.2.2-statistics.md](./2.2.2-statistics.md) |
| **2.2.3** | ID 分配 · 溢出隐患 | [2.2.3-id-allocation.md](./2.2.3-id-allocation.md) |

**阅读顺序**：**2.2.1 → 2.2.2 → 2.2.3** → [2.3 索引](../2.3-compare-and-exchange/2.3-compare-and-exchange.md)

---

## Demo

[2.2-fetch-and-modify-demo.rs](./code/2.2-fetch-and-modify-demo.rs)

章索引：[本章学习笔记.md](../本章学习笔记.md)""",
)

w(
    "2.2-fetch-and-modify/2.2.1-multi-thread-progress.md",
    """## 2.2.1 Progress Reporting from Multiple Threads

> 书 §2.2 · 索引：[2.2](./2.2-fetch-and-modify.md) · 承接 [2.1.2 load/store](../2.1-atomic-load-store/2.1.2-progress-reporting.md)

---

多线程更新**同一**进度计数时，不能用 `store`（后写覆盖先写），应 **`fetch_add(1, Relaxed)`** 等 RMW 指令。

Demo：[code/2.2-fetch-and-modify-demo.rs](./code/2.2-fetch-and-modify-demo.rs)""",
)

w(
    "2.2-fetch-and-modify/2.2.2-statistics.md",
    """## 2.2.2 Example: Statistics（统计）

> 书 §2.2 · 索引：[2.2](./2.2-fetch-and-modify.md)

---

对**同一原子变量**可 `fetch_add`、`fetch_max` 等；**多个原子变量**（如 `num_done` 与 `total_time`）分别更新时，他线程仍可能看到中间态 → 业务不可接受时用 **`Mutex`** 打包。

Demo：[code/2.2-fetch-and-modify-demo.rs](./code/2.2-fetch-and-modify-demo.rs)""",
)

w(
    "2.2-fetch-and-modify/2.2.3-id-allocation.md",
    """## 2.2.3 Example: ID Allocation（ID 分配）

> 书 §2.2 · 索引：[2.2](./2.2-fetch-and-modify.md) · 防溢出见 [2.3.1 CAS](../2.3-compare-and-exchange/2.3.1-id-no-overflow.md)

---

`fetch_add(1)` 发号简单，但 **`u32::MAX` 后 wrapping 回 0**；事后 `assert!` **拦不住**其他线程继续加。关键 ID → [2.3.1 compare_exchange 循环](../2.3-compare-and-exchange/2.3.1-id-no-overflow.md).

Demo：`2.3-compare-and-exchange/code/2.3-compare-and-exchange-id-allocator-demo.rs`（与 2.3 共用）""",
)

w(
    "2.3-compare-and-exchange/2.3-compare-and-exchange.md",
    """## 2.3 Compare-and-Exchange（CAS）

> 书 §2.3 · 专题：[CAS与Fetch-Modify专题.md](../CAS与Fetch-Modify专题.md)  
> **本节正文已拆分**；按 **2.3.1 → 2.3.2** 顺序阅读。

---

**语义**：当前值 == 期望值 → 写入新值并 `Ok(旧值)`；否则不改并 `Err(当前值)`。循环中用 **`compare_exchange_weak`**（允许虚假失败）。

## §2.3 子笔记索引

| 子节 | 主题 | 笔记 |
|------|------|------|
| **2.3.1** | ID 分配无溢出 | [2.3.1-id-no-overflow.md](./2.3.1-id-no-overflow.md) |
| **2.3.2** | 一次性懒加载 | [2.3.2-lazy-one-time-init.md](./2.3.2-lazy-one-time-init.md) |

Demo：[code/2.3-compare-and-exchange-id-allocator-demo.rs](./code/2.3-compare-and-exchange-id-allocator-demo.rs)

章索引：[本章学习笔记.md](../本章学习笔记.md)""",
)

w(
    "2.3-compare-and-exchange/2.3.1-id-no-overflow.md",
    """## 2.3.1 ID Allocation Without Overflow

> 书 §2.3 · 索引：[2.3](./2.3-compare-and-exchange.md) · 前：[2.2.3 fetch_add 局限](../2.2-fetch-and-modify/2.2.3-id-allocation.md)

---

`loop`：`load` → 若超阈值 `panic` → `compare_exchange_weak(id, id+1)`，失败则更新期望值重试。**检查与更新在同一原子语义内**（Fetch-Update 模式）。

Demo：[code/2.3-compare-and-exchange-id-allocator-demo.rs](./code/2.3-compare-and-exchange-id-allocator-demo.rs)""",
)

w(
    "2.3-compare-and-exchange/2.3.2-lazy-one-time-init.md",
    """## 2.3.2 Lazy One-Time Initialization

> 书 §2.3 · 索引：[2.3](./2.3-compare-and-exchange.md) · 承接 [2.1.4 Relaxed 懒加载](../2.1-atomic-load-store/2.1.4-lazy-init.md)

---

仅 `compare_exchange(0, new_key)` **成功者**写入；`Err` 则丢弃自己的 key，使用他人已写入值。适用于**唯一随机密钥**等只能初始化一次的场景。

Demo：[code/2.1-atomic-load-store-lazy-init-demo.rs](../2.1-atomic-load-store/code/2.1-atomic-load-store-lazy-init-demo.rs)（与 2.1.4 共用）""",
)

w(
    "2.4-summary/2.4-summary.md",
    f"""## 2.4 Summary（本章小结）

> 书 §2.4 · 章索引：[本章学习笔记.md](../本章学习笔记.md) · [全书目录](../../全书目录-与实体书一致.md)

---

## 一、一句话回顾

**Relaxed** 原子 API：`load`/`store`、`fetch_*`、`compare_exchange`；单变量原子性 ≠ 多变量业务一致；溢出用 CAS 循环；可见性与顺序见 **第 3 章**。

---

## 二、各节对照

| 书 § | 主题 | 笔记 |
|------|------|------|
| 2.1 | Load / Store | [2.1 索引](../2.1-atomic-load-store/2.1-atomic-load-store.md) |
| 2.2 | Fetch-Modify | [2.2 索引](../2.2-fetch-and-modify/2.2-fetch-and-modify.md) |
| 2.3 | CAS | [2.3 索引](../2.3-compare-and-exchange/2.3-compare-and-exchange.md) |

---

## 三、日常规避法则

1. 优先 `scope` / `Mutex` / channel，勿默认手写 atomics  
2. 两原子变量业务一致 → `Mutex`  
3. 全局 ID → CAS + 循环防 overflow  
4. `unsafe` 须写清 Safety 不变量

---

## 四、Demo · 延伸

| 文件 | 内容 |
|------|------|
| [2.4-summary-demo.rs](./code/2.4-summary-demo.rs) | load/store/fetch/CAS 速览 |
| Ch03 `3.3-memory-order-options` | SeqCst / Acquire-Release |
| Ch03 `3.4-fences` | Fence |

```bash
cargo build --manifest-path {M}
```

---

## 五、与 LLVM IR（可选）

最小函数拷入 `05/04_Learn-LLVM-17/src/lib.rs`，`--emit=llvm-ir` 观察 `atomicrmw`、`cmpxchg` → `ir_samples/atomic_ir/`""",
)

print("wrote ch02 section notes")
