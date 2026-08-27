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
| **`CLONE_FILES`** | 文件描述符表 | 默认共享 fd；一线程 `close(fd)` 全组可见；`exec` 时自动复制断开共享 |
| **`CLONE_SIGHAND`** | 信号处理函数表（`sa_handler`） | 共享；但**信号掩码 per-task 独立**（`task_struct->blocked`，各线程可独立 `pthread_sigmask`） |
| **`CLONE_THREAD`** | 同 **线程组**（`tgid`） | `pthread_create` 必设；线程退出**不发 SIGCHLD** → 不能 `wait`，要用 `pthread_join` |
| **`CLONE_FS`** | `fs_struct`（cwd、root、umask） | 共享；一线程 `chdir` 全组生效 |
| **`CLONE_SETTLS`** | 线程局部存储区 | `pthread` TLS，实现 `__thread` 变量 |
| **`CLONE_PARENT_SETTID`** | 内核把新线程 TID 写回父进程用户态地址 | `pthread_create` 返回的 TID 来源 |
| **`CLONE_CHILD_CLEARTID`** | 线程退出时清空指定地址并触发 futex 唤醒 | `pthread_join` 阻塞等待的底层机制 |

> `CLONE_CHILD_CLEARTID` 是 `pthread_join` 的核心：join 方在用户态 futex 等待该地址，线程退出时内核清地址 + futex wake，join 方即被唤醒。避免了「线程退出通知」走信号路径——这也解释了为什么 `CLONE_THREAD` 路径下终止信号传 **0**（不发 SIGCHLD）。

#### LinuxThreads → NPTL（历史）

| 缺陷 | LinuxThreads | NPTL |
|------|--------------|------|
| 线程组 | 每线程各自独立 PID（无 `CLONE_THREAD`） | 同 `tgid`，`getpid()` 返回一致 |
| 信号 | 每线程独立进程，信号语义混乱 | per-thread 掩码 + 线程组定向，符合 POSIX |
| `getpid()` | 每线程返回不同值 | 线程组内一致 |
| 管理 | `wait` 能收线程（不符合 POSIX） | 线程用 `pthread_join`，进程用 `wait` |

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

#### 三种创建路径对比

| 维度 | `fork()` | `vfork()` | `pthread_create()`（NPTL） |
|------|----------|-----------|------------------------------|
| **底层 clone flags** | 仅 `SIGCHLD`（无任何 CLONE_* 共享） | `CLONE_VM \| CLONE_VFORK \| SIGCHLD` | `CLONE_VM \| CLONE_FILES \| CLONE_SIGHAND \| CLONE_THREAD \| CLONE_FS \| CLONE_SETTLS \| CLONE_PARENT_SETTID \| CLONE_CHILD_CLEARTID`，终止信号 **0** |
| **地址空间 mm** | 独立（COW 写时复制） | **共享**（不 COW，父挂起） | 共享 |
| **fd 表 files** | 复制（引用计数 +1） | 复制 | 共享 |
| **信号处理表 sighand** | 复制 | 复制 | 共享（但**信号掩码 per-task 独立**） |
| **cwd / root / umask** | 复制（不设 CLONE_FS） | 复制 | 共享（一线程 chdir 全组生效） |
| **线程组归属** | 新 tgid（=新 PID） | 新 tgid | **同 tgid**（getpid 返回相同值） |
| **父进程行为** | 父子并发 | 父**阻塞**到子 `_exit/exec` | 父子并发 |
| **子退出通知** | `SIGCHLD` 给父 → `wait` 回收 | `SIGCHLD` 给父 → `wait` 回收 | **不发信号**；靠 `CLONE_CHILD_CLEARTID` 触发 futex 唤醒 → `pthread_join` |
| **终止信号参数** | `SIGCHLD` | `SIGCHLD` | **0** |
| **PID/TID** | 子获新 PID（=TID） | 子获新 PID | 子获新 **TID**，TGID 不变 |
| **典型用途** | shell 起子进程、daemon | 几乎不用（fork+COW 已够快） | 多线程并行、IO 线程 |
| **现代评价** | 通用 | **不推荐**，posix_spawn 替代 | 标准 POSIX 线程 |

> 「终止信号 SIGCHLD vs 0」是上表的关键差异：`CLONE_THREAD` 置位时内核不向父发 SIGCHLD，所以才不能用 `wait` 收线程，只能 `pthread_join`（底层由 `CLONE_CHILD_CLEARTID` 清地址 + futex 唤醒实现）。

#### fork 与多线程的三个坑

| 坑 | 现象 | 根因 |
|----|------|------|
| **1. fork 只保留调用线程** | 多线程父进程 fork 后，子进程里**只有调用 fork 的那条 LWP 存活**，其余线程全部消失 | fork 只复制当前 `task_struct`，不复制同组的其他线程；子进程的 `mm` 仍指向原地址空间（COW），但其他线程的调度实体没了 |
| **2. `exit()` 杀全组** | 线程里调 `exit()` 会终止**整个线程组所有线程** | `exit()` 作用于 tgid（进程级）；只杀当前 LWP 要用 `pthread_exit()`，它只销毁当前 `task_struct` 并触发 `CLONE_CHILD_CLEARTID` 的 futex 唤醒 |
| **3. 共享地址 → 需同步** | LWP 间全局变量直接互相可见，不加锁就 race | `CLONE_VM` 让所有线程共用同一份页表；fork 出的子进程是独立 `mm`，改了也不影响父（COW） |

> 坑 1 的实战后果：fork 后子进程**不要随便碰 pthread 库**——锁的状态是从父进程那一刻冻结的快照，如果别的线程正持着锁，子进程里那把锁永远等不到释放 → 死锁。POSIX 规定只有 async-signal-safe 函数能在 fork 后的子进程里调用，就是这个道理。

#### 对比表

| 类型 | 地址空间 | 创建者 | 典型用途 |
|------|----------|--------|----------|
| **普通进程** | 独立 `mm` | `fork` / `clone` 无 VM 共享 | shell、daemon |
| **NPTL 线程** | 共享 `mm` | `pthread` → `clone` | 并行计算、IO 线程 |
| **内核线程** | 无用户 `mm` | `kthread_create` | 延迟工作、驱动 bottom half |

**HFT：** 行情解码、发单、风控通常在 **用户线程 + CPU 绑核（`pthread_setaffinity` / `sched_setaffinity`）**；延迟抖动也常来自 **内核线程** 与 **软中断** 争用同一 CPU — 见 **Ch 4、8**。线程数 ≈ 逻辑核数（或略多 IO 线程），避免 CFS 上大量 runnable 线程互相抢份额。

→ [§3.2 task_struct](./section-3.2-进程描述符与任务结构.md) · [Ch 4 §4.6 RT](../../chapter-04-process-scheduling/notes/section-4.6-实时调度策略.md) · [07 TLPI Ch29 线程](../../../03-linux-userspace-api/chapter-29-threads-intro/notes) · [07 TLPI Ch30 线程同步](../../../03-linux-userspace-api/chapter-30-thread-synchronization/notes)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** Linux 线程和进程在内核层面有什么区别？clone() 的 flags 如何控制？

<details><summary>答案</summary>

Linux 内核没有「线程」概念，线程 = 共享资源的进程。clone(CLONE_VM | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD) 创建线程：共享地址空间/文件表/信号处理/线程组。clone(不设这些 flag) 创建进程。`pthread_create` 底层调 clone，`fork` 底层也调 clone 但不设共享 flag。

</details>

**Q2.** NPTL 相比 LinuxThreads 改进了什么？为什么对 HFT 重要？

<details><summary>答案</summary>

LinuxThreads 用单独进程实现线程，每线程一个 PID，信号处理混乱。NPTL（Native POSIX Thread Library）：线程共享 PID（gettid 区分），信号符合 POSIX（per-thread 信号掩码），线程创建快 10x+。HFT 多线程依赖 NPTL 的 futex 快速锁和 per-thread 信号。

</details>

</details>
---
