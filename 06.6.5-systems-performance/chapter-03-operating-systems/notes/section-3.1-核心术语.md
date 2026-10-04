## 3.1 核心术语

> [3.2 内核基础与核心概念](./section-3.2-内核基础与核心概念.md) · [3.3–3.4 内核演进与 Linux 特性](./section-3.3-3.4-内核演进与-Linux-特性.md) · [3.5 其他系统模型](./section-3.5-其他系统模型.md) · [3.6 内核比较](./section-3.6-内核比较.md)

---

### 本节讲什么

全书的地基词汇表。3.2 会展开每个概念的机制细节，本节先把**术语之间的关系**摆清楚——哪个包含哪个、哪个和哪个是并列、哪个经常被混淆。性能分析的每一句话都由这些词构成，词用错了，结论必然错。

### 要点

| # | 要点 | 一句话展开 |
|---|------|-----------|
| 1 | OS ⊃ 内核 ⊃（调度/内存/网络/FS…） | 「OS 慢」不是诊断，「内核网络栈 softirq 高」才是 |
| 2 | 进程 ⊃ 线程（Linux 下线程=共享资源的任务） | 观测口径完全不同：ps 看 PID、top -H 看 TID |
| 3 | **mode switch ≠ context switch** | 换特权级 vs 换执行流；syscall 只做前者 |
| 4 | syscall 是「接口」，不是「开销本体」 | 开销在模式切换 + 路径长度 + 可能的连锁反应 |
| 5 | 中断分「硬件异步」与「同步陷阱」两族 | 缺页是 trap 不是 IRQ——它不走中断控制器的路径 |
| 6 | 每个术语都有对应观测入口 | 没有观测入口的术语在性能分析里没有发言权 |

---

### 一、术语总表（含性能问法与观测入口）

| 术语 | 含义 | 性能分析中的问法 | 观测入口 |
|------|------|------------------|----------|
| **OS** | 操作系统（内核 + 用户态系统程序） | 版本、补丁、发行版差异 | `uname -a`、`/etc/os-release` |
| **Kernel** | 内核（OS 的特权核心） | 调度、内存、网络栈在哪一层 | `/proc/version`、`perf` |
| **Process** | 进程（资源分配单位） | 哪个 PID、多少内存、多少 fd | `ps`、`/proc/PID/` |
| **Thread** | 线程（调度执行单位） | 哪个 TID 吃 CPU、是否绑核 | `top -H`、`pidstat -t` |
| **Context switch** | 上下文切换（换执行流） | `pidstat -w`、run queue、cache 冷 | `vmstat` cs 列、`perf sched` |
| **Mode switch** | 模式切换（用户↔内核特权级） | syscall 路径开销 | `perf stat`（周期数）|
| **System call** | 系统调用（用户请求内核服务） | `read`/`write`/`send`/`mmap`/`clone`… | `strace -c`（调试）、`syscount` |
| **Hardware interrupt** | 硬中断（外设异步通知） | IRQ 分布、网卡收包路径 | `/proc/interrupts` |
| **Softirq / 软中断** | 中断下半部（延后处理） | 收包、块层完成路径的税 | `/proc/softirqs`、`mpstat` %soft |
| **Trap / 陷阱** | 同步异常（syscall 入口、缺页） | 缺页是性能尖刺来源之一 | `perf stat` page-faults |
| **Scheduler** | 调度器（决定下一个跑谁） | 排队延迟、不公平性 | `runqlat`、`/proc/PID/sched` |

---

### 二、术语关系图

```
┌─────────────── OS（操作系统）───────────────┐
│  ┌──────────── Kernel（内核）────────────┐  │
│  │   调度器 ──── 内存管理 ──── 网络栈     │  │
│  │      │            │          │        │  │
│  │   VFS/块层 ──── 中断子系统 ── …       │  │
│  └───────────────┬───────────────────────┘  │
│                  │ syscall（接口）           │
│  ┌───────────────┴───────────────────────┐  │
│  │       用户态：进程、线程、libc、运行时  │  │
│  └───────────────────────────────────────┘  │
└─────────────────────────────────────────────┘

进程（资源单位：地址空间、fd 表、权限）
 └── 线程（执行单位：寄存器、栈、调度实体）
      ├── 共享：地址空间、堆、全局数据
      └── 独有：栈、TLS、程序计数器
```

---

### 三、最易混淆的一对：Context Switch vs Mode Switch

这对术语在口语里经常被混用（「syscall 引起上下文切换」——错），但机制与代价完全不同：

| 维度 | Mode Switch（模式切换） | Context Switch（上下文切换） |
|------|------------------------|------------------------------|
| 换什么 | **特权级**（用户 ring3 ↔ 内核 ring0） | **执行流**（当前 CPU 上跑的任务） |
| 何时发生 | 每次 syscall 进出、异常 | 时间片耗尽、阻塞唤醒、更高优先级抢占 |
| 保存/恢复 | 少量寄存器 + 切换到内核栈 | **全套寄存器 + 切换页表（可能）+ 换调度上下文** |
| 代价量级 | ~几十-上百 ns（受 KPTI 放大） | ~1-10 µs **外加间接代价** |
| 间接代价 | TLB 部分失效（KPTI 更重） | **cache/TLB 全冷**——后续 miss 才是真正的痛 |
| 观测 | 指令数（perf stat） | `vmstat` cs 列、`pidstat -w`、`perf sched` |

关键关系：**syscall 只需要 mode switch；但 syscall 若阻塞（如 read 等盘），就升级为 context switch**。这是「热路径少 syscall」的真正理由——不只是省模式切换的几十 ns，更是避免「syscall 阻塞 → 让出 CPU → cache 全冷」这条连锁链。展开见 [3.2 内核基础](./section-3.2-内核基础与核心概念.md)。

**HFT 延迟分解里的应用**：tick 路径某段延迟不明时，按这四类依次排除——**syscall（模式切换）**、**缺页（trap → 可能 majflt 读盘）**、**中断/softirq（被别人插队）**、**调度切换（runqueue 排队）**。四类各有专属观测工具（上表右列），排除法一条条走。

---

### 四、中断两族：异步 IRQ vs 同步 Trap

| 维度 | 硬件中断（IRQ） | 同步陷阱（Trap） |
|------|----------------|------------------|
| 触发时机 | **异步**——外设随时打断 | **同步**——当前指令执行的结果 |
| 例子 | 网卡收包、磁盘完成、时钟 | syscall 入口、缺页、除零 |
| 与当前任务的关系 | 「被插队」——打断的是无辜路人 | 「自作自受」——当前指令引发 |
| 性能含义 | 延迟抖动源（隔离/定向 IRQ affinity） | 路径成本（syscall 数、缺页数可控） |
| 观测 | `/proc/interrupts`、`mpstat` %irq | `perf stat` 的 page-faults、syscount |

把缺页归类为 trap 而非 IRQ 的实际意义：**缺页次数是程序行为的函数**（预分配、mlock、touch 内存策略都可以控），而 IRQ 到达率是外部世界的函数（行情风暴来了就是来了）。两者的调优手段完全不同——前者改代码，后者改隔离。

softirq 是第三类存在（中断的「下半部」），机制上既不是 IRQ 也不是 trap，而是在开中断的上下文里延后处理的内核工作——收包路径的 NET_RX softirq 是 HFT 最常见的一课，见 [3.2 中断机制](./section-3.2-内核基础与核心概念.md)。

---

### HFT / 嵌入式关联

- **延迟分解四象限**：syscall / 缺页 / 中断 / 调度切换——tick 路径任何一段不明延迟，先归类再选工具。这与 [6.5 三级方法漏斗](../../chapter-06-cpus/notes/section-6.5-性能分析方法论.md)、[10.5 网络方法论](../../chapter-10-network/notes/section-10.5-分析方法论.md) 的下钻流程是同一件事在不同子系统上的投影。
- **嵌入式注意**：嵌入式语境里「进程/线程」术语常被 RTOS 的「任务（task）」替代，且无用户/内核态之分（裸机或 flat memory）——但「切换代价 = 直接保存 + cache 冷却」的分析框架完全通用。
- **观测入口列是刻意加的**：性能工程师的词汇表必须带工具——只会背定义不会测量的术语是死的。

---

### 衔接

- 下一节 [3.2 内核基础与核心概念](./section-3.2-内核基础与核心概念.md)：本节每个术语的机制展开（syscall 完整路径、中断上下/下半部、调度器、内存管理、VFS）。
- 用户/内核分界与 syscall 流程的源码级深挖：[LKD Ch 5 系统调用](../../../05-linux-kernel/chapter-05-system-calls/)。
- context switch 的间接代价（cache 冷却）在 [Ch 6 CPU](../../chapter-06-cpus/) 展开；缺页在 [Ch 7 内存](../../chapter-07-memory/) 展开。

---

### 常见陷阱

1. 「syscall 引起上下文切换」——错；syscall 引起的是模式切换，只有阻塞/被抢占才升级为上下文切换
2. 缺页当硬件中断处理——缺页是同步 trap，其次数是程序行为的函数（可控），不是外部世界的函数
3. 混用 OS 与内核——「OS 慢」没有信息量；性能结论必须落到内核子系统或用户态代码
4. 只背定义不配观测——每个术语都要知道用什么工具测量它，否则诊断无从下手
5. 把线程当进程看——`ps` 只看到 PID 级，多线程程序里吃 CPU 的可能是某个 TID；用 `top -H` / `pidstat -t`

<details>
<summary>自测题（点击展开）</summary>

1. mode switch 和 context switch 的区别是什么？
   <details><summary>答</summary>mode switch 换特权级（用户↔内核，~几十-上百 ns，syscall 必经）；context switch 换执行流（保存全套寄存器+可能换页表，~1-10µs 外加 cache/TLB 全冷的间接代价）。syscall 只需前者；syscall 阻塞才升级为后者</details>
2. 为什么热路径要少 syscall？真正的痛在哪里？
   <details><summary>答</summary>模式切换本身几十 ns 只是显性成本；真正怕的是连锁链——syscall 阻塞 → 让出 CPU → 调度切换 → cache/TLB 全冷 → 后续一串 miss。省 syscall 是为了不触发这条链</details>
3. 缺页属于哪类中断？这个归类对调优意味着什么？
   <details><summary>答</summary>同步 trap（当前指令引发），不是异步 IRQ——所以缺页次数是程序行为的函数，可以用预分配/mlock/touch 策略控制；而 IRQ 到达率是外部世界的函数，只能靠隔离/IRQ affinity 应对</details>
4. HFT tick 路径一段不明延迟，如何用术语归类法排除？
   <details><summary>答</summary>按四象限依次排除：syscall（模式切换，syscount/perf stat）、缺页（trap，perf stat page-faults）、中断/softirq（mpstat %irq/%soft、/proc/interrupts）、调度切换（runqlat、vmstat cs）——每类有专属工具，逐项归零</details>
5. softirq 为什么既不算 IRQ 也不算 trap？
   <details><summary>答</summary>softirq 是硬中断的「下半部」——延后到开中断上下文里处理的内核工作（如收包 NET_RX）；它是 IRQ 的衍生品但不再由硬件触发，和当前指令也无关</details>

</details>


---

← [本章导读](../README.md)
