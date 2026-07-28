## ⑤ Linux 的线程实现 · NPTL & clone

#### 用户态线程 = 共享资源的进程

Linux **没有** 单独的「线程」内核对象类型：

| 观点 | 实现 |
|------|------|
| 线程 | **恰好共享部分资源** 的 **普通进程**（独立 `task_struct`） |
| 创建 | **`clone()`** syscall + **标志位** 指定共享项 |
| 用户库 | **NPTL**（Native POSIX Thread Library）— `pthread_*` 封装 `clone` |

#### 常见 CLONE_* 标志

| 标志 | 共享内容 | pthread 近似 |
|------|----------|--------------|
| **`CLONE_VM`** | 地址空间（`mm`） | 默认线程 |
| **`CLONE_FILES`** | 文件描述符表 | 默认共享 fd |
| **`CLONE_SIGHAND`** | 信号处理函数表 | 同进程信号语义 |
| **`CLONE_THREAD`** | 同 **线程组**（`tgid`） | `pthread_create` 必设 |
| **`CLONE_FS`** | `fs_struct`（cwd、root） | 可共享 |
| **`CLONE_SETTLS`** | 线程局部存储区 | `pthread` TLS |

```
进程（线程组 tgid = 100）
┌─────────────────────────────────────┐
│  共享：mm · files · sighand（可选）    │
├──────────┬──────────┬───────────────┤
│ task pid │ task pid │ task pid      │
│   100    │   101    │   102         │  ← 各有一个 task_struct
└──────────┴──────────┴───────────────┘
```

#### NPTL 要点

| 特性 | 说明 |
|------|------|
| **1:1 模型** | 每个 pthread 对应一个内核调度实体 |
| **futex** | 用户态锁 + 内核快速路径睡眠/唤醒 |
| **`gettid()`** | 系统调用返回 **内核 PID**（线程 ID） |
| **`pthread_self()`** | 用户库 ID，≠ 内核 tid |

```c
/* 用户态：pthread 底层即 clone（概念示意） */
int clone_start(void *(*fn)(void *), void *arg, void *stack, void *tls);
/* glibc 组装 CLONE_VM|CLONE_FILES|CLONE_SIGHAND|CLONE_THREAD|... */
```

#### 内核线程 · Kernel Threads

| 特点 | 说明 |
|------|------|
| 仅在内核空间运行 | **`mm == NULL`**，借用上一任务的 `active_mm` |
| 后台任务 | `ksoftirqd`、`kworker`、写回、`migration` |
| 创建 | **`kthread_create()` / `kthread_run()`** — 仅内核可建 |
| 与用户线程关系 | **同一套调度器** — [Ch 4](../../chapter-04-process-scheduling/) |

```
kthread（无用户地址空间）
  task_struct ──► mm = NULL
                ──► 只跑 kernel text 中的函数
```

#### 对比表

| 类型 | 地址空间 | 创建者 | 典型用途 |
|------|----------|--------|----------|
| **普通进程** | 独立 `mm` | `fork` / `clone` 无 VM 共享 | shell、daemon |
| **NPTL 线程** | 共享 `mm` | `pthread` → `clone` | 并行计算、IO 线程 |
| **内核线程** | 无用户 `mm` | `kthread_create` | 延迟工作、驱动 bottom half |

**HFT：** 行情解码、发单、风控通常在 **用户线程 + CPU 绑核（`pthread_setaffinity` / `sched_setaffinity`）**；延迟抖动也常来自 **内核线程** 与 **软中断** 争用同一 CPU — 见 **Ch 4、8**。线程数 ≈ 逻辑核数（或略多 IO 线程），避免 CFS 上大量 runnable 线程互相抢份额。

→ [§3.2 task_struct](./section-3.2-进程描述符与任务结构.md) · [Ch 4 §4.6 RT](../../chapter-04-process-scheduling/notes/section-4.6-实时调度策略.md) · [07 TLPI Ch22–24 线程](../../../../07-The-Linux-Programming-Interface/chapter-22-threads-intro/notes.md) · [07 TLPI Ch33 futex/同步](../../../../07-The-Linux-Programming-Interface/chapter-23-thread-synchronization/notes.md)

---
