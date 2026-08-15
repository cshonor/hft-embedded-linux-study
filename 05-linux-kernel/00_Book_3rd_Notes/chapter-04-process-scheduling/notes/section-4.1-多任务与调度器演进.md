## ① 多任务与调度器演进

### 总起

Linux = **抢占式多任务（Preemptive Multitasking）**：  
调度器可在 **安全时机强制剥夺** 当前占用 CPU 的任务，**不必** 等它主动 `yield()` / `schedule()`。

| 对立面 | 说明 |
|--------|------|
| **协作式多任务** | 任务必须 **自愿** 交还 CPU；一个死循环即可卡死整机（早期 Windows 3.x 一类模型） |

---

### 术语表

| 术语 | 通俗解释 |
|------|----------|
| **调度器 scheduler** | 内核决策模块：谁上 CPU、何时切、跑多久（份额） |
| **抢占 preemption** | 强行打断当前任务，把 CPU 分给别的任务 |
| **运行队列 runqueue** | 每 CPU（常见）上的就绪集合；只收 **`TASK_RUNNING`（可运行）** 的 task；**不在队列里 = 没资格被选中上 CPU** |
| **时间片 / 份额** | **O(1)**：偏固定时间片；**CFS**（现行默认）：不用固定片，按 **`vruntime` 虚拟运行时间** 分 CPU **份额** |

---

### 为何必须抢占

计算密集型任务若死循环占满 CPU 且系统 **不能抢占**：

| 后果 | |
|------|--|
| 网络服务、日志、ssh、终端 | **饿死（starvation）** |
| 交互 / 尾延迟 | 只能「等别人做完」→ 爆炸 |

**场景：** 写一个空转死循环的 Rust/C 程序 —  

| 系统 | 现象 |
|------|------|
| **抢占式 Linux** | 定时器/抢占点会打断它，终端与其它进程仍能跑 |
| **纯协作式** | 一启动该循环，整机可能无法操作 |

生产 OS 必须抢占；协作式只适合教学玩具。

---

### 串到 `task_struct` / fork / exec

```
fork() → 新 task_struct + 新 PID
              │
              ▼
进程就绪（TASK_RUNNING）→ 进入某 CPU 的 runqueue / CFS 红黑树
              │
              ▼
调度器选下一个 → 上下文切换（换寄存器 / mm …）
              │
抢占从哪来？  常由 时钟中断 / 唤醒更高优先级 等置 need_resched
              → 在安全点强制切换
```

| 你已有的概念 | 调度侧落点 |
|--------------|------------|
| `task_struct` | 调度实体挂在上面（CFS 还有 `sched_entity`） |
| `fork` | 造出可被调度的新身份；就绪后进 runqueue |
| `execve` | **不换 PID**，换用户映像；调度身份仍是这个 task |
| PID / FD | 身份与资源钥匙 — 见 [§3.8](../../chapter-03-process-management/notes/section-3.8-身份PID与资源FD.md) |

**一句话：** 调度器不「创建程序」；它只在 **已存在的 task** 里挑谁跑。创建靠 `fork`/`clone`，换程序靠 `exec`。

---

### 历史脉络

```
早期简单调度器（线性扫描 / 简单轮询）
    │
    ▼
O(1) 调度器（2.5 / 2.6 前期）
    │  每 CPU 优先级数组 · 选下一个 O(1)
    │  大机扩展性好；交互/公平感常被诟病
    │
    ▼
CFS（2.6.23+，Completely Fair Scheduler）
    │  默认公平调度 · 按权重分 CPU 比例 · vruntime + 红黑树
    │
    ▼
现代：CFS + 独立 RT 类 +（可选）Deadline / 带宽控制
```

| 调度器 | 选下一个 | 强项 | 弱项 |
|--------|----------|------|------|
| **O(1)** | 优先级位图 · **O(1)** | 海量任务、吞吐 | 交互延迟、公平性争议 |
| **CFS** | 红黑树最左 · **O(log n)** | 交互响应、公平比例 | 硬实时仍靠 **RT 类**（见 4.6） |

---

### 抢占分层（后续精读预告）

| 层级 | 含义 | 低延迟含义 |
|------|------|------------|
| **用户态抢占** | 从内核返回用户态前可换人 | 基础 |
| **内核抢占**（如 `CONFIG_PREEMPT`） | 内核路径在可抢占点也可被换下 | **降低调度延迟**；HFT/服务器常关注（细节 → [4.5](./section-4.5-抢占与上下文切换.md)） |

持锁、关中断、原子上下文等处 **不能随意抢占** — 否则破坏同步。

---

### 和「你看到的」工具对应

| 用户态现象 | 内核侧 |
|------------|--------|
| `top`/`htop` 运行态、切换 | 调度器在跑谁、切多勤 |
| `perf sched`、延迟尖刺 | 抢占点、唤醒延迟、锁持有 |
| `chrt` / `nice` | RT 类 / CFS 权重 |

**HFT：** 不要指望「调一下 nice」解决微秒级抖动；热路径通常是 **隔离核 + RT/FIFO + 中断亲和**，CFS 负责「其余世界别饿死」。

→ 下一节：[4.2 策略](./section-4.2-调度策略.md) · [4.3 CFS](./section-4.3-Linux-调度算法.md) · [4.5 抢占与切换](./section-4.5-抢占与上下文切换.md)  
→ [15 SysPerf §3.2 O(1)→CFS](../../../../15-systems-performance/chapter-03-operating-systems/notes/section-3.2-内核基础与核心概念.md)

### 常见陷阱

1. 把 O(1) 调度器当现代默认——2.6.23 起 CFS 取代 O(1)，6.6 起 EEVDF 取代 CFS
2. 混淆「多任务」和「多线程」——多任务是 OS 级概念（多个进程分时），多线程是进程内并发
3. 以为调度器只看优先级——CFS 看 vruntime（按权重标准化的虚拟运行时间），不看绝对优先级

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** Linux 调度器从 O(1) 到 CFS 到 EEVDF 的演进动机分别是什么？

<details><summary>答案</summary>

O(1) → CFS：O(1) 的启发式优先级奖励（sleep 时间 → bonus）复杂且不公平，CFS 用 vruntime 实现精确公平。CFS → EEVDF：CFS 的唤醒抢占不够精确，EEVDF 用虚拟截止时间 + eligibility 提供延迟保证，且解决了某些公平性 corner case。

</details>

**Q2.** 多任务（multitasking）的两种模式？Linux 用哪种？

<details><summary>答案</summary>

① 协作式（cooperative）：进程主动让出 CPU（如早期 Windows 3.1）。② 抢占式（preemptive）：内核强制切换。Linux 用抢占式——时钟中断 + 优先级强制切换，即使进程不让出也会被调度。内核中 `CONFIG_PREEMPT` 进一步允许内核态抢占。

</details>

**Q3.** HFT 为什么不依赖 CFS 公平调度？

<details><summary>答案</summary>

CFS 面向通用公平性，不保证延迟上限。HFT 用 `SCHED_FIFO`（RT 策略）+ 绑核 + `isolcpus`，完全绕过 CFS。RT 线程独占 CPU 直到阻塞或被更高优先级抢占。

</details>

</details>


> ↔ [ULK Ch7 §1 本章定位](../../../../19-linux-kernel-deep/chapter-07-process-scheduling/notes/section-1-本章定位.md)
---
