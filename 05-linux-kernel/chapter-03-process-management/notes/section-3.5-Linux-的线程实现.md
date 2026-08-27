## ⑤ Linux 的线程实现 · NPTL & clone

#### 用户态线程 = 共享资源的进程

Linux **没有** 单独的「线程」内核对象类型：

| 观点 | 实现 |
|------|------|
| 线程 | **恰好共享部分资源** 的 **普通进程**（独立 `task_struct`） |
| 创建 | **`clone()`** syscall + **标志位** 指定共享项 |
| 用户库 | **NPTL**（Native POSIX Thread Library）— `pthread_*` 封装 `clone` |

> 经典表述：**进程是资源分配的最小单位，线程是 CPU 调度的最小单位**。Linux 里这两者都是 `task_struct`——调度器只看 task_struct，资源边界由 clone 标志决定。

#### 常见 CLONE_* 标志

| 标志 | 共享内容 | pthread 近似 |
|------|----------|--------------|
| **`CLONE_VM`** | 地址空间（`mm`） | 默认线程 |
| **`CLONE_FILES`** | 文件描述符表 | 默认共享 fd；一线程 `close(fd)` 全组可见；`exec` 时自动复制断开共享 |
| **`CLONE_SIGHAND`** | 信号处理函数表（`sa_handler`） | 共享；但**信号掩码 per-task 独立**（`task_struct->blocked`，各线程可独立 `pthread_sigmask`） |
| **`CLONE_THREAD`** | 同 **线程组**（`tgid`） | `pthread_create` 必设；线程退出**不发 SIGCHLD** → 不能 `wait`，要用 `pthread_join` |
| **`CLONE_FS`** | `fs_struct`（cwd、root、umask） | 共享；一线程 `chdir` 全组生效 |
| **`CLONE_SETTLS`** | 线程局部存储区 | `pthread` TLS，实现 `__thread` 变量 |
| **`CLONE_SYSVSEM`** | System V 信号量 undo 列表（`sem_adj`） | 共享；NPTL 实际也传这个 flag |
| **`CLONE_PARENT_SETTID`** | 内核把新线程 TID 写回父进程用户态地址 | `pthread_create` 返回的 TID 来源 |
| **`CLONE_CHILD_CLEARTID`** | 线程退出时清空指定地址并触发 futex 唤醒 | `pthread_join` 阻塞等待的底层机制 |

#### flags 的结构：低 8 位是退出信号，高位才是开关

`flags` 就是一个**比特掩码整数**：每个 `CLONE_*` 宏只有单独一位置 1；用户态用**按位或 `|`** 把多个开关叠加进同一个参数，内核用**按位与 `&`** 逐个检测：

```c
/* 用户态组合 */
flags = CLONE_VM | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD | SIGCHLD;
/* 内核检测（copy_process 路径） */
if (flags & CLONE_VM)
        p->mm = current->mm;      /* 共享地址空间 */
else
        p->mm = dup_mm(current);  /* 复制 + COW */
```

```
flags 整数
├─ 低 8 bit（1 字节）：子任务死亡时发给父进程的信号（fork 填 SIGCHLD；
│                     NPTL 线程因 CLONE_THREAD 路径填 0，不发信号）
└─ 其余高位比特：各 CLONE_* 开关（fork 全 0 → 全部资源复制）
```

⚠️ **不能乱拼——内核有依赖校验**（`copy_process` 中检查，违反直接 `-EINVAL`）：

- 开 `CLONE_SIGHAND` **必须**同时开 `CLONE_VM`（共享信号处理表的前提是共享地址空间）
- 开 `CLONE_THREAD` **必须**同时开 `CLONE_SIGHAND`（进而必须 `CLONE_VM`）

> 所以 NPTL 传的是一整套成体系的 flag 组合，不是随便开一两个。依赖链条：`CLONE_THREAD → CLONE_SIGHAND → CLONE_VM`。

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
│ pid=100  │ pid=101  │ pid=102       │  ← 各有一个 task_struct（pid 字段=TID）
│(组长=主) │ (TID)    │ (TID)         │     tgid 全部 = 100
└──────────┴──────────┴───────────────┘
```

#### LWP：共享 vs 私有清单

LWP（Light-Weight Process）在内核里是**一份完整独立的 `task_struct`**，调度器直接调度它：

| 同组 LWP **共享** | 每个 LWP **私有** |
|-------------------|-------------------|
| `mm` 地址空间（同一份页表） | `task_struct` 本身 + 唯一 **TID** |
| `files` 文件描述符表 | **用户栈**（`clone` 时分配的新栈） |
| `fs`（cwd、root、umask） | **内核栈**（`task_struct->stack`） |
| `sighand` 信号处理函数表 | 寄存器上下文（调度切换时保存） |
| | **信号掩码**（`task_struct->blocked`） |
| | **TLS**（`CLONE_SETTLS` 设置各自线程局部存储） |

> 观察命令：**`ps -aL`** —— 同一个 PID（=TGID）下面列出多条不同 LWP-ID（=TID）的任务；**LWP 列打印的就是 `gettid()` 的值**。
> `getpid()` 返回 **TGID**（同组全一样），`gettid()` 返回 **TID**（每条不同）。

##### 字段名陷阱：`task_struct->pid` 存的其实是 TID

| 内核字段 | 实际语义 | 用户态取值 |
|----------|----------|-----------|
| `task_struct->pid` | **TID**（任务唯一 ID，调度器真正识别的 ID） | `gettid()` |
| `task_struct->tgid` | 线程组 ID（= 用户说的"进程号"） | `getpid()` |

- **fork 普通进程**：`pid == tgid`（自己是组长，组里只有自己）
- **同进程多条 LWP**：`tgid` 全部相同，`pid`（LWP 号）各自不同
- **主线程也是 LWP**：`pid == tgid`，它是**线程组组长**（`group_leader`）
- ⚠️ 术语边界：**LWP 特指开了 `CLONE_VM` 共享地址空间的任务**（用户态说的"线程"）；fork 出的普通进程一般**不算** LWP。LWP 是口语/教科书称呼，不是内核结构体名——内核里统一叫 task（`task_struct`）。

⚠️ **`CLONE_THREAD` 是"是线程"的判定标志，不是 `CLONE_VM`**：不开 `CLONE_THREAD`、只开 `CLONE_VM`，产物**不是 POSIX 线程**——它有自己独立的 TGID（新进程），不归属任何线程组，父进程要 `wait` 回收它。它只是一个「共享地址空间的独立进程」（广义上算轻量级进程，但 NPTL 不这么干，平时几乎碰不到）。用户态看是两个进程共享内存，内核看是两条无关 task_struct 恰好指向同一个 `mm`。

> 通俗比喻：`clone` 是万能工厂，flags 是一排开关——**全关（fork）→ 普通进程；把一排共享开关全开（pthread_create）→ LWP**。内核本身分不清"进程/线程"，眼里只有 task_struct；**进程和线程只是用户层的抽象概念**，边界由开关组合决定。

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

> 源码印证：glibc `nptl/pthread_create.c`（github.com/bminor/glibc）里能看到这组 flags 的组装现场。

#### 三类线程模型 · 用户态 / 内核态 / 混合（教科书视角）

| 模型 | 调度在哪 | 内核视角 | 一条线程阻塞 syscall | 代表 |
|------|----------|---------|---------------------|------|
| **用户态线程**（N:1） | 用户空间库 | **只见 1 个 task_struct**，看不见内部线程 | **整个进程全卡住**（内核把 syscall 阻塞算在整个进程头上） | 早期 green threads、**goroutine 的 G** |
| **内核态线程**（1:1） | 内核调度器 | **每条线程一个 task_struct**（LWP） | 只阻塞当前那条，其余继续跑 | **NPTL pthread**（`clone` 创建 LWP） |
| **混合**（M:N） | 用户库 + 内核协作 | M 条用户线程映射到 N 条 LWP | 只阻塞所在的 LWP | Go runtime（G-M-P） |

- **切换开销**：用户态线程切换不陷内核、最快；内核态线程切换要走 syscall 路径，开销大一些
- **NPTL 不是第三种模型** —— 它是「内核态线程（1:1）」的 POSIX 用户库实现：`pthread_create` → `clone` → 一条内核可见的 LWP
- ⚠️ **术语陷阱**：**内核态线程**（kernel-level thread = LWP，有用户地址空间）≠ **内核线程**（kernel thread = `kthread`，`mm=NULL` 的纯内核执行流，见下节）——一字之差，两个概念

> 通俗比喻：用户态线程 = **一个工人**手上切换多个活，他去打水（阻塞 `read()`），所有活全停；NPTL/LWP = 向工厂申请**多个工人**共享一间办公室（同一份 `mm`），一人打水别人继续干，且工厂（内核）认识每个工人。

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
| **底层 clone flags** | 仅 `SIGCHLD`（无任何 CLONE_* 共享） | `CLONE_VM \| CLONE_VFORK \| SIGCHLD` | `CLONE_VM \| CLONE_FILES \| CLONE_SIGHAND \| CLONE_THREAD \| CLONE_FS \| CLONE_SETTLS \| CLONE_SYSVSEM \| CLONE_PARENT_SETTID \| CLONE_CHILD_CLEARTID`，终止信号 **0** |
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

**Q3.** 三类线程模型中，Go 的 goroutine 和 Linux 的 pthread 各属于哪类？内核态线程和内核线程是一回事吗？

<details><summary>答案</summary>

goroutine 是 M:N 混合模型：G 是用户态调度单元，由 Go runtime 调度，M 才映射到内核 LWP。Linux pthread 是 1:1 内核态线程：pthread_create 直接 clone 出一条内核可见的 LWP（NPTL 只是内核态线程模型的 POSIX 库实现，不是第三种模型）。内核态线程（LWP，有用户地址空间）≠ 内核线程（kthread，mm=NULL 的纯内核执行流）。

</details>

**Q4.** 只开 `CLONE_VM`（不开 `CLONE_THREAD`）clone 出的任务是什么？它和同组线程怎么区分？`getpid()` 和 `gettid()` 在一个多线程进程里各返回什么？

<details><summary>答案</summary>

只是**共享地址空间的独立进程**：有自己的 TGID，不属于任何线程组，父进程须 `wait` 回收——不是 POSIX 线程。「是不是线程」由 `CLONE_THREAD` 判定，不是 `CLONE_VM`。多线程进程里 `getpid()` 返回 TGID（所有线程相同），`gettid()` 返回各线程自己的 TID（每条不同）。补充：主线程也是 LWP，`pid == tgid`，是线程组组长。

</details>

</details>
---
