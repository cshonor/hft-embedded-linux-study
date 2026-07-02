# 2. 应用程序基础 (Application Fundamentals)

### 线程管理 (Thread Management)

| 模式 | 说明 | HFT 常见 |
|------|------|----------|
| 每连接一线程 | 简单，扩展差 | 少见 |
| **线程池** | 固定 worker 处理任务 | 网关、IO |
| **事件驱动 / SEDA** | 单线程或分阶段 pipeline | **策略 hot path** |
| 绑核 | 减少迁移 | `taskset` / isolcpus |

**BPF 关联：** `threadsnoop` 看 **何时 `pthread_create`**；`threaded` 看 **是否仅一线程在跑**。

### 锁 (Locks)

`libpthread`：**mutex、rwlock、spinlock** — **锁竞争** → 线程阻塞 → Off-CPU 在 **futex**。

| 现象 | 工具线索 |
|------|----------|
| P99 抖、CPU 不高 | `offcputime` 栈含 `futex` / `pthread_mutex_lock` |
| 谁等谁 | **`pmlock` / `pmheld`** |

→ [15-HFT ch07 无锁](../15-HFT-Low-Latency-Practice/chapter-07-无锁数据结构与内存布局.md)

### 休眠 (Sleeps)

应用显式 **`sleep` / `nanosleep` / `usleep`** — 常为 **人为延迟** 或错误轮询。

| 工具 | 作用 |
|------|------|
| **`naptime`** | 追踪 `nanosleep(2)` — 抓 **代码里写死的 sleep** |

**HFT：** 热路径不应有 sleep；`naptime` 一击即中。

---
