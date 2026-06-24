# Ch 3 · 进程管理 · Process Management

> **Linux Kernel Development 3rd** · Robert Love · **选读**  
> 本章定位：Linux **进程抽象** 的内核实现 — `task_struct`、状态机、`fork`+COW、`clone` 线程模型、退出与僵尸。为 **Ch 4 调度**、**Ch 15 地址空间** 打底。

---

## 本节结构

| 节 | 主题 | 带走什么 |
|----|------|----------|
| **① 进程概念** | 执行期程序 + 资源 | **`fork` → `exec`** |
| **② 描述符** | `task_struct` · task list | Slab · `thread_info` |
| **③ 进程状态** | `state` 五态 | 可中断 vs 不可中断睡眠 |
| **④ 创建与 COW** | `fork()` 优化 | **写时拷贝** |
| **⑤ 线程** | 无专用线程类型 | **`clone()` + 标志** · 内核线程 |
| **⑥ 终结** | `exit` · 僵尸 · 孤儿 | **`wait` · reparent → init** |

---

### ① 进程的概念 · The Process

**进程** = 处于 **执行期** 的程序 + **相关资源** 的集合：

| 资源示例 | 说明 |
|----------|------|
| 打开的文件 | fd 表 |
| 挂起的信号 | 待递送信号 |
| 处理器状态 | 寄存器、PC |
| 内存地址空间 | 虚拟内存布局 |
| 一个或多个执行线程 | Linux 中仍属「进程」模型 |

#### 典型创建路径（Unix 两步）

```
fork()  ──► 复制现有进程（子进程）
   │
   └──► exec() 族 ──► 加载新可执行文件，替换映像
```

| 调用 | 作用 |
|------|------|
| **`fork()`** | 复制进程 — 父子并发 |
| **`exec*()`** | 换程序 — 常接在 `fork` 之后 |

**HFT 对照：** 热路径网关多用 **线程池 + `clone`/`pthread`**，极少 **每连接 `fork`**（页表/COW 仍懂成本即可）。

→ [01-CSAPP Ch8 fork/exec](../../01-CSAPP-3rd/chapter-08-exceptional-control-flow/) · [Ch9 COW](../../01-CSAPP-3rd/chapter-09-virtual-memory/notes/section-9.8-内存映射mmap.md) · [07-TLPI 进程章](../../07-The-Linux-Programming-Interface/)

---

### ② 进程描述符与任务结构

内核用 **任务队列（task list）** — **环形双向链表** — 串联所有进程。

| 结构 | 说明 |
|------|------|
| **`task_struct`** | **进程描述符** — 管理该进程所需的 **全部信息**（书中约 **~1.7KB**） |
| 链表项 | 每个进程一个 `task_struct` 节点 |

#### 分配与 `thread_info`

| 设计 | 目的 |
|------|------|
| **Slab 分配器** 动态分配 `task_struct` | 高效、缓存友好 |
| **`thread_info`** 放在 **进程内核栈底/顶** | 省寄存器、快速定位当前任务 |
| `thread_info` 内指针 | 指向对应 **`task_struct`** |

```
内核栈（4K/8K）
┌──────────────────┐
│   栈帧 / 局部变量   │
├──────────────────┤
│  thread_info     │ ──► task_struct（Slab）
└──────────────────┘
```

→ **Ch 6** 内核数据结构 · **Ch 12** Slab/mm · **Ch 2** 小栈约束

---

### ③ 进程状态 · Process State

`task_struct` 的 **`state`** 字段 — 任一时刻必居 **下列五态之一**（经典 3rd 表述）：

| 状态 | 宏 | 含义 |
|------|-----|------|
| **运行** | `TASK_RUNNING` | 正在 CPU 上跑，或在 **运行队列** 里等 CPU |
| **可中断睡眠** | `TASK_INTERRUPTIBLE` | 阻塞等事件；**收到信号可提前唤醒** → 可回到运行 |
| **不可中断睡眠** | `TASK_UNINTERRUPTIBLE` | 阻塞等事件；**信号不能唤醒** — 常用于必须等完的 I/O |
| **被跟踪** | `__TASK_TRACED` | 被调试器 **`ptrace`** 跟踪 |
| **停止** | `__TASK_STOPPED` | 收到 **`SIGSTOP` / `SIGTSTP`** 等而暂停 |

#### 状态迁移（简化）

```
        调度选中
  RUNNING ◄──────────── 就绪队列
     │                      ▲
     │ 等待资源/睡眠          │ 信号/事件就绪
     ▼                      │
 INTERRUPTIBLE / UNINTERRUPTIBLE
     │
     │ ptrace
     ▼
 TRACED / STOPPED
```

**HFT / 观测：** `D` 状态（不可中断睡眠）过多 → 磁盘/NFS 等阻塞拖慢整条流水线；`perf`/`ps` 与 **Ch 4 运行队列** 联读。

→ [02 SysPerf §3.2 进程与调度](../../02-Systems-Performance-2nd/chapter-03-operating-systems/notes/section-3.2-内核基础与核心概念.md)

---

### ④ 进程创建与写时拷贝 · Copy-on-Write

Unix 创建 = **`fork` + `exec`**。Linux 对 **`fork()`** 用 **写时拷贝（COW）** 优化：

| 阶段 | 行为 |
|------|------|
| **`fork` 瞬间** | **不**复制整个物理地址空间 |
| 初始 | 父子 **共享** 同一套物理页的 **只读映射** |
| **首次写入** | 内核 **才真正复制** 该页 — 写者得到私有副本 |

| 收益 | 说明 |
|------|------|
| **少复制** | 许多 `fork` 后立刻 `exec`，大量页从未被写 |
| **快创建** | 延续 Unix「**极快 fork**」传统 |

```
fork 后：
  父 ──┬── 同一物理页（只读标记）
  子 ──┘
       │
  一方 write ──► 内核复制该页，各自可写
```

→ **Ch 15** 进程地址空间 · VMA · 页表细节

---

### ⑤ Linux 的线程实现

#### 用户态线程 = 共享资源的进程

Linux **没有** 单独的「线程」内核对象类型：

| 观点 | 实现 |
|------|------|
| 线程 | **恰好共享部分资源** 的 **普通进程** |
| 创建 | **`clone()`** syscall + **标志位** 指定共享项 |

| 常见 `clone` 标志 | 共享内容 |
|-------------------|----------|
| **`CLONE_VM`** | 地址空间 |
| **`CLONE_FILES`** | 文件描述符表 |
| **`CLONE_SIGHAND`** | 信号处理 |
| **`CLONE_THREAD`** | 同一线程组（POSIX 线程语义） |

`pthread_create` 在用户库底层即组装这些标志调用 **`clone`**。

#### 内核线程 · Kernel Threads

| 特点 | 说明 |
|------|------|
| 仅在内核空间运行 | **无独立用户地址空间** |
| 后台任务 | 如 `ksoftirqd`、`kworker`、写回线程 |
| 创建 | **只能由内核**（或其他内核线程）创建 — 如 `kthread_create` |

**HFT 对照：** 行情/发单在 **用户线程 + 绑核**；延迟抖动也常来自 **内核线程** 与 **软中断** 争用同一 CPU — 见 **Ch 4、8**。

→ [08-1 Day 12 多任务](../../08-system-low-level-hands-on/08-1-30days-os/notes/day-12-多任务.md)（教学 OS 分层对照）

---

### ⑥ 进程终结 · Process Termination

#### 触发

| 路径 | 说明 |
|------|------|
| **`exit()`** | 主动退出 |
| **无法处理的信号** | 默认动作终止 |

#### 清理：`do_exit()`

释放 **内存、文件、信号** 等大部分资源 — 进程进入 **僵尸态**。

| 阶段 | 状态 / 动作 |
|------|-------------|
| 退出运行 | **`do_exit()`** 回收绝大部分资源 |
| **僵尸 `EXIT_ZOMBIE`** | 仍占 **`task_struct` + 内核栈** — 保留 **退出码** 给父进程 |
| 父进程 **`wait()` / `waitpid()`** | 取走退出状态 |
| **`release_task()`** | 彻底释放描述符 |

```
子进程 exit ──► ZOMBIE（等父 wait）
                    │
父 wait ───────────► release_task() ──► 描述符消失
```

#### 孤儿进程与 reparenting

| 情况 | 内核行为 |
|------|----------|
| **父先于子退出** | 子成为 **孤儿进程** |
| **reparenting** | 交给线程组内其他线程，或 **`init`（PID 1）** 收养 |
| 目的 | 保证仍有人 **`wait`** — **避免僵尸永久遗留** |

**运维：** 僵尸泛滥 → 父进程 bug（未 `wait`）；`init` 会定期回收其收养子进程。

→ [01-CSAPP Ch8 僵尸/孤儿](../../01-CSAPP-3rd/chapter-08-exceptional-control-flow/)

---

### Ch 3 小结

| 问题 | 答案 |
|------|------|
| 进程是什么？ | **执行中程序 + 资源集合** |
| 内核怎么表示？ | **`task_struct`** on **task list** · Slab + **`thread_info`** |
| 五态？ | **RUNNING / INTERRUPTIBLE / UNINTERRUPTIBLE / TRACED / STOPPED** |
| `fork` 为何快？ | **COW** — 写时才复制物理页 |
| 线程？ | **`clone` 标志共享资源** — 无单独线程结构 |
| 内核线程？ | 仅内核态、无用户地址空间、内核创建 |
| 退出后为何还有 PID？ | **僵尸** — 等父 **`wait`**；孤儿由 **init** 收养 |

---

### 检查单

- [ ] 画出 **`fork` → COW → exec** 与 **pthread ≈ clone(CLONE_VM|…)** 的关系
- [ ] 区分 **可中断 / 不可中断睡眠** 与信号行为
- [ ] 解释 **僵尸** 占什么、**`wait`** 做什么
- [ ] 知道 **内核线程** 与用户线程不是同一层抽象
- [ ] 能对照 HFT：**少 fork、多线程、注意 D 状态与 kthread 争核**

---

## 相关章节

- 上一章：[chapter-02-内核入门.md](./chapter-02-内核入门.md)
- 下一章：[chapter-04-进程调度.md](./chapter-04-进程调度.md)
- 本模块导读：[README.md](./README.md) · [OUTLINE.md](./OUTLINE.md)
