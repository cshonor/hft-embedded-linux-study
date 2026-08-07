#!/usr/bin/env python3
"""07-linux-kernel LKD 🔴 章节新手化增强脚本
为 6 个🔴章节共 46 篇笔记添加 常见陷阱(3) + 折叠自测题(3)
"""
import os

BASE = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "00_Book_3rd_Notes")

def make_block(traps, quiz):
    lines = ["### 常见陷阱", ""]
    for i, t in enumerate(traps, 1):
        lines.append(f"{i}. {t}")
    lines += ["", "---", "", "<details>", "<summary>自测题（点击展开）</summary>", ""]
    for i, (q, a) in enumerate(quiz, 1):
        lines.append(f"**Q{i}.** {q}")
        lines.append("")
        lines.append("<details><summary>答案</summary>")
        lines.append("")
        lines.append(a)
        lines.append("")
        lines.append("</details>")
        lines.append("")
    lines.append("</details>")
    lines.append("")
    return "\n".join(lines)

def insert_before_last_sep(content, block):
    stripped = content.rstrip()
    lines = stripped.split("\n")
    for i in range(len(lines) - 1, -1, -1):
        if lines[i].strip() == "---":
            lines.insert(i, block)
            return "\n".join(lines) + "\n"
    return stripped + "\n\n" + block + "\n"

NOTES = {}

# === Ch4 Process Scheduling (7 notes) ===
C4 = "chapter-04-process-scheduling/notes"
NOTES[f"{C4}/section-4.1-多任务与调度器演进.md"] = (
    ["把 O(1) 调度器当现代默认——2.6.23 起 CFS 取代 O(1)，6.6 起 EEVDF 取代 CFS",
     "混淆「多任务」和「多线程」——多任务是 OS 级概念（多个进程分时），多线程是进程内并发",
     "以为调度器只看优先级——CFS 看 vruntime（按权重标准化的虚拟运行时间），不看绝对优先级"],
    [("Linux 调度器从 O(1) 到 CFS 到 EEVDF 的演进动机分别是什么？",
      "O(1) → CFS：O(1) 的启发式优先级奖励（sleep 时间 → bonus）复杂且不公平，CFS 用 vruntime 实现精确公平。CFS → EEVDF：CFS 的唤醒抢占不够精确，EEVDF 用虚拟截止时间 + eligibility 提供延迟保证，且解决了某些公平性 corner case。"),
     ("多任务（multitasking）的两种模式？Linux 用哪种？",
      "① 协作式（cooperative）：进程主动让出 CPU（如早期 Windows 3.1）。② 抢占式（preemptive）：内核强制切换。Linux 用抢占式——时钟中断 + 优先级强制切换，即使进程不让出也会被调度。内核中 `CONFIG_PREEMPT` 进一步允许内核态抢占。"),
     ("HFT 为什么不依赖 CFS 公平调度？",
      "CFS 面向通用公平性，不保证延迟上限。HFT 用 `SCHED_FIFO`（RT 策略）+ 绑核 + `isolcpus`，完全绕过 CFS。RT 线程独占 CPU 直到阻塞或被更高优先级抢占。")]
)

NOTES[f"{C4}/section-4.2-调度策略.md"] = (
    ["混淆 SCHED_OTHER 和 SCHED_FIFO——OTHER 是 CFS 管的普通分时，FIFO 是 RT 调度器管的实时",
     "以为 nice 值 -20 到 19 对应优先级 -20 到 19——内部映射为 static_prio = 120 + nice，范围 [100, 139]",
     "在 RT 策略下以为 nice 还有效——RT 策略看 rt_priority (1-99)，不看 nice"],
    [("六个调度策略（SCHED_OTHER/BATCH/IDLE/FIFO/RR/DEADLINE）各自适用什么场景？",
      "OTHER：普通分时进程（默认）。BATCH：CPU 密集型批处理（低交互）。IDLE：极低优先级后台任务。FIFO：实时 FIFO，无时间片。RR：实时轮转，有时间片。DEADLINE：基于 EDF，需指定 runtime/deadline/period，优先级最高。HFT 用 FIFO。"),
     ("nice 值到 CPU 权重的映射是怎么实现的？",
      "nice [-20,19] → static_prio = 120 + nice → 查 sched_prio_to_weight[] 表。nice 0 = weight 1024，nice +5 = 335，nice -5 = 3121。每差 1 级 nice，权重比约 1.25。两个进程 A(nice=0, weight=1024) B(nice=5, weight=335) 的 CPU 比例 ≈ 1024:335 ≈ 3:1。"),
     ("`SCHED_FIFO` 的 `rt_priority` 怎么设置？范围是什么？",
      "通过 `sched_setscheduler(pid, SCHED_FIFO, &param)` 设置，`param.sched_priority` 范围 1-99（0 表示非 RT）。数字越大优先级越高。HFT 通常设 99（最高），配合 `isolcpus` 独占 CPU。注意需要 `CAP_SYS_NICE` 权限。`/proc/sys/kernel/sched_rt_runtime_us` 默认 950000 限制 RT 占用 95% CPU 时间，HFT 可设 -1 禁用。")]
)

NOTES[f"{C4}/section-4.3-Linux-调度算法.md"] = (
    ["把 vruntime 当物理运行时间——vruntime = Δt × (1024/weight)，权重高涨得慢",
     "以为红黑树按优先级排序——按 vruntime 排序，最左节点 = vruntime 最小 = 最该运行的",
     "混淆调度周期（sched_latency）和时间片——CFS 没有固定时间片，每人分到的时间随就绪任务数变化"],
    [("CFS 的 vruntime 公式和物理含义？",
      "`vruntime += Δt × (1024 / weight)`。物理含义：把实际运行时间按权重标准化。权重高（nice 低）的进程 vruntime 涨得慢，更容易被选中（红黑树最左 = vruntime 最小）。效果：权重 1024 vs 512 的两进程，物理 CPU 比例 ≈ 2:1。"),
     ("CFS 选下一个进程的复杂度是多少？为什么用红黑树？",
      "O(log n)。红黑树按 vruntime 排序，最左节点 = vruntime 最小 = 下一个运行。插入/删除 O(log n)。如果用排序数组则 O(n log n) 插入。红黑树是 Linux 内核通用数据结构（`struct rb_node`），CFS/CFS 运行队列/内存管理都在用。"),
     ("HFT 用 SCHED_FIFO 后 CFS 还起作用吗？",
      "不起作用。调度优先级：DEADLINE > RT(FIFO/RR) > CFS(OTHER/BATCH/IDLE)。有就绪 RT 任务时，CFS 全体让路。HFT 交易线程用 SCHED_FIFO + 绑核后，CFS 的公平性/vruntime/红黑树都不影响该线程。但需注意 RT throttling（`rt_runtime_us`）。")]
)

NOTES[f"{C4}/section-4.4-休眠与唤醒.md"] = (
    ["把「休眠」当浪费 CPU——休眠释放 CPU 给其他任务，是高效的资源利用",
     "混淆 `TASK_INTERRUPTIBLE` 和 `TASK_UNINTERRUPTIBLE`——前者可被信号唤醒，后者不可",
     "以为唤醒后立即运行——唤醒只是把进程放回运行队列，是否立即运行取决于调度器"],
    [("`TASK_INTERRUPTIBLE` 和 `TASK_UNINTERRUPTIBLE` 的区别？",
      "INTERRUPTIBLE：可被信号唤醒（`kill -9` 有效），进程可响应异步事件。UNINTERRUPTIBLE：不可被信号唤醒（`kill -9` 无效），通常在等磁盘 I/O 等不可中断操作。`TASK_KILLABLE` 是 2.6.25+ 新增：可被致命信号唤醒但不被普通信号打断。HFT 避免在热路径上进入 UNINTERRUPTIBLE（D 状态，无法 kill）。"),
     ("唤醒抢占的阈值是什么？为什么需要？",
      "`sched_wakeup_granularity`（默认 1ms）。唤醒的进程 vruntime 比 current 小这个阈值时才抢占。太低（0）会导致频繁抢占（thrashing）；太高会导致交互延迟。HFT 用 SCHED_FIFO 不走 CFS，不受此影响。"),
     ("HFT 如何避免热路径上的休眠/唤醒延迟？",
      "① 预分配所有资源（内存/连接/FD），避免运行时等资源。② 无锁队列代替 mutex（mutex 阻塞 = 休眠）。③ DPDK 用户态轮询代替 epoll_wait（epoll_wait 阻塞 = 休眠）。④ `SCHED_FIFO` 确保唤醒后立即运行（RT 优先级 > CFS）。")]
)

NOTES[f"{C4}/section-4.5-抢占与上下文切换.md"] = (
    ["混淆用户态抢占和内核态抢占——用户态在 syscall 返回/中断返回时抢占，内核态需要 CONFIG_PREEMPT",
     "以为上下文切换只是保存寄存器——还要切换 mm_struct（页表/TLB）、FPU 状态、TLS",
     "忽略 switch_mm 的开销——页表切换 + TLB 刷新是 context switch 中最昂贵的部分"],
    [("用户态抢占和内核态抢占的触发点分别是什么？",
      "用户态抢占：① syscall 返回用户态时检查 need_resched。② 中断返回用户态时检查。内核态抢占（CONFIG_PREEMPT）：① 中断返回内核态时（preempt_count==0）。② preempt_enable() 时。③ 显式 schedule()。无 CONFIG_PREEMPT 时内核代码不可被抢占（除非自愿 schedule）。"),
     ("context_switch 的两个核心步骤及各自开销？",
      "① switch_mm()：切换页表（写 CR3）+ TLB 刷新。开销 ~1-3us（TLB 重建最贵）。内核线程不需要（lazy TLB）。② switch_to()：保存/恢复寄存器 + FPU 状态。开销 ~100-300ns。总 context switch 延迟 ~1-5us。HFT 用绑核 + isolcpus 消除切换。"),
     ("HFT 如何测量和消除 context switch？",
      "测量：① `perf stat -e context-switches`。② `/proc/[pid]/status` 的 `voluntary_ctxt_switches` / `nonvoluntary_ctxt_switches`。③ `pidstat -w -p [pid]`。消除：① 绑核 + SCHED_FIFO。② `isolcpus` 隔离。③ `mlockall` 防 swap 引起的切换。④ 无锁设计避免 mutex 阻塞。目标：0 nonvoluntary switches。")]
)

NOTES[f"{C4}/section-4.6-实时调度策略.md"] = (
    ["混淆 SCHED_FIFO 和 SCHED_RR——FIFO 无时间片（跑到阻塞），RR 有时间片轮转",
     "以为 RT 优先级 99 就是最高——SCHED_DEADLINE 优先级更高（EDF 算法）",
     "忽略 RT throttling——RT 线程默认被限制在 95% CPU 时间，HFT 需要禁用"],
    [("SCHED_FIFO 和 SCHED_RR 的核心区别？",
      "FIFO：同优先级内先到先服务，无时间片，跑到阻塞/被更高优先级抢占/yield。RR：同优先级内轮转，每个任务有时间片（默认 100ms），片耗尽后轮到同优先级的下一个。HFT 通常用 FIFO——只有一个 RT 线程独占核，不需要轮转。"),
     ("RT throttling 机制是什么？HFT 怎么处理？",
      "`/proc/sys/kernel/sched_rt_period_us`（默认 1s）和 `sched_rt_runtime_us`（默认 0.95s）限制 RT 线程在每 period 内最多跑 runtime 时间。超出后 RT 被节流，CFS 接管。HFT 设 `sched_rt_runtime_us=-1` 禁用节流。注意：禁用后如果 RT 线程死循环会锁死 CPU，设 `RLIMIT_RTTIME` 做安全网。"),
     ("HFT 使用 SCHED_FIFO 的完整配置步骤？",
      "```c\n// 1. 绑核\ncpu_set_t cs; CPU_ZERO(&cs); CPU_SET(2, &cs);\nsched_setaffinity(0, sizeof(cs), &cs);\n// 2. RT 优先级 99\nstruct sched_param sp = { .sched_priority = 99 };\nsched_setscheduler(0, SCHED_FIFO, &sp);\n// 3. 锁内存\nmlockall(MCL_CURRENT | MCL_FUTURE);\n// 4. 内核启动参数: isolcpus=2 nohz_full=2 rcu_nocbs=2\n```")]
)

NOTES[f"{C4}/section-4.7-与调度相关的系统调用.md"] = (
    ["混淆 nice() 和 setpriority()——nice() 是相对调整（+=inc），setpriority() 是绝对设置",
     "以为 sched_setaffinity() 需要 root——只需要 CAP_SYS_NICE 设置 RT 策略，affinity 任何进程都可以设",
     "在 SCHED_FIFO 下用 sched_yield()——FIFO 下 yield 移到同优先级队列末尾，可能不立即重新运行"],
    [("nice(inc)、setpriority()、sched_setscheduler() 的区别？",
      "nice(inc)：当前进程 nice += inc，受 RLIMIT_NICE 限制。setpriority(PRIO_PROCESS, pid, prio)：设置指定进程的 nice 绝对值。sched_setscheduler(pid, policy, &param)：切换调度策略和 RT 优先级。HFT 用 sched_setscheduler(SCHED_FIFO, 99) 设最高 RT 优先级。"),
     ("sched_setaffinity() 和 isolcpus 有什么区别？",
      "sched_setaffinity()：运行时设置进程可运行的 CPU 集合，其他进程仍可被调度到这些核。isolcpus=2：启动时从调度器可运行集合移除 2 号核，普通任务不会调度到 2，需要手动 taskset/sched_setaffinity 把 RT 线程放上去。isolcpus 更彻底（连 kworker/RCU 都不走），affinity 更灵活。HFT 两者都用。"),
     ("如何用 sched_getaffinity() 和 sched_setaffinity() 绑核？",
      "```c\n#include <sched.h>\ncpu_set_t mask;\nCPU_ZERO(&mask);\nCPU_SET(2, &mask);  // 绑到 2 号核\nsched_setaffinity(0, sizeof(mask), &mask);  // 0=当前进程\n// 验证\ncpu_set_t get;\nsched_getaffinity(0, sizeof(get), &get);\nprintf(\"CPU 2: %d\\n\", CPU_ISSET(2, &get));  // 1\n```")]
)

# === Ch7 Interrupts (7 notes) ===
C7 = "chapter-07-interrupts/notes"
NOTES[f"{C7}/section-7.1-中断的概念.md"] = (
    ["混淆中断（异步硬件触发）和异常（同步指令触发）——中断不可预测，异常由特定指令引起",
     "以为中断处理可以睡眠——hard IRQ 不能睡眠（无 task_struct），threaded IRQ 可以",
     "忽略中断延迟对 HFT 的影响——一次 NIC 中断可能抢占交易线程数十微秒"],
    [("中断和异常的根本区别？",
      "中断：异步、硬件触发（NIC/键盘/定时器），CPU 在两条指令间响应。异常：同步、指令执行触发（#PF 缺页/#GP 段错误/#DE 除零），CPU 在出错指令处响应。Fault 可恢复（重执行），Trap 用于调试（继续），Abort 不可恢复。"),
     ("为什么 hard IRQ 不能睡眠？",
      "Hard IRQ 运行在中断上下文：无 task_struct（不可调度）、无进程栈（用中断栈）、preempt_count 中 HARDIRQ 位被设置。schedule() 检查 preempt_count != 0 会 panic。解决方案：threaded IRQ（request_threaded_irq），hard IRQ 只确认硬件 + 唤醒内核线程，实际处理在线程中可睡眠。"),
     ("HFT 如何减少中断对交易线程的干扰？",
      "① `/proc/irq/[n]/smp_affinity` 绑中断到非交易核。② `service irqbalance stop`。③ `isolcpus` 隔离交易核。④ `nohz_full` 减少定时器中断。⑤ DPDK 用户态轮询完全绕过中断。⑥ NAPI 轮询模式（网络收包）。")]
)

NOTES[f"{C7}/section-7.2-中断处理程序.md"] = (
    ["在中断处理函数中做耗时操作——应拆成上半部（确认硬件）+ 下半部（实际处理）",
     "以为中断处理函数可以返回任意值——必须返回 IRQ_HANDLED 或 IRQ_NONE",
     "混淆 request_irq() 和 request_threaded_irq()——前者不能睡眠，后者可在线程中处理"],
    [("中断处理函数（上半部）的职责是什么？应该避免做什么？",
      "职责：① 确认硬件（ACK/EOI）。② 读取硬件数据。③ 调度下半部（softirq/tasklet/workqueue）。避免：① I/O 操作（磁盘/网络）。② 内存分配（GFP_ATOMIC 可能失败）。③ 持 mutex（不能睡眠）。④ 长时间计算。原则：越快越好（<10us），复杂工作交给下半部。"),
     ("request_irq() 和 request_threaded_irq() 的区别？",
      "request_irq(dev, handler, flags, name, dev_id)：handler 在 hard IRQ 上下文运行，不能睡眠。request_threaded_irq(dev, hard_fn, thread_fn, flags, name, dev_id)：hard_fn 确认硬件 + 唤醒内核线程，thread_fn 在线程中处理（可睡眠/持 mutex/做 I/O）。现代驱动推荐 request_threaded_irq。"),
     ("IRQ_HANDLED 和 IRQ_NONE 的区别？返回 IRQ_NONE 会怎样？",
      "IRQ_HANDLED：中断被本驱动处理。IRQ_NONE：中断不属于本驱动（共享 IRQ 场景）。内核统计 spurious IRQ：如果同一 IRQ 连续 99900 次返回 IRQ_NONE 而不到 100 次 IRQ_HANDLED，内核禁用该 IRQ 并打印警告。HFT 应确保中断处理函数正确判断中断来源。")]
)

NOTES[f"{C7}/section-7.3-上半部与下半部.md"] = (
    ["混淆上半部和下半部——上半部在 hard IRQ 上下文（不可睡眠），下半部在 softirq/进程上下文",
     "以为 tasklet 还被推荐——tasklet 已 deprecated，推荐 workqueue 或 threaded IRQ",
     "在 softirq 中调用 mutex_lock()——softirq 不能睡眠，只能用 spinlock"],
    [("上半部和下半部的分工原则？",
      "上半部（hard IRQ）：① 确认硬件 ACK。② 读取时间敏感数据。③ 调度下半部。必须 <10us。下半部（softirq/tasklet/workqueue）：① 大部分中断处理工作。② 可做 I/O、分配内存（workqueue 可睡眠）。③ 可被中断抢占但不能被同类型 softirq 抢占（per-CPU）。"),
     ("softirq、tasklet、workqueue 三种下半部机制如何选择？",
      "softirq：编译时静态注册，性能最高，不能睡眠。tasklet：基于 softirq，动态注册，同类型不并发，已 deprecated。workqueue：在 kworker 线程运行，可睡眠/持 mutex，最灵活。选择：需要睡眠 → workqueue。需要高性能 → softirq（或 NAPI）。不再推荐 → tasklet（用 workqueue 或 threaded IRQ 替代）。"),
     ("HFT 中下半部对延迟有什么影响？",
      "NIC 收包走 NET_RX_SOFTIRQ，在 hard IRQ 返回或 ksoftirqd 中执行。如果 softirq 积压，收包延迟增大。排查：`/proc/softirqs` 看 NET_RX 计数。解决：① 绑 softirq 到非交易核（RPS/RFS）。② NAPI 轮询模式。③ DPDK 完全绕过 softirq。④ `nohz_full` 减少软中断频率。")]
)

NOTES[f"{C7}/section-7.4-注册与编写中断处理程序.md"] = (
    ["忘记在中断处理函数中 disable 该 IRQ——共享 IRQ 场景下可能导致重复触发",
     "混淆 IRQF_SHARED 和 IRQF_TRIGGER_*——前者允许共享，后者指定触发方式（上升沿/下降沿）",
     "在模块卸载时忘记 free_irq()——导致空指针/中断风暴"],
    [("request_irq() 的 flags 参数中 IRQF_SHARED 和 IRQF_TRIGGER_* 的含义？",
      "IRQF_SHARED：允许多个设备共享同一 IRQ 号（所有共享者都收到中断，各自判断是否属于自己的设备）。IRQF_TRIGGER_RISING/FALLING/HIGH/LOW：指定中断触发方式（边沿/电平）。PCI 设备通常用 MSI（消息信号中断），不需要 IRQF_SHARED。"),
     ("共享 IRQ 的中断处理函数怎么判断中断是否属于自己？",
      "读取设备的中断状态寄存器（ISR），如果 pending 位为 0 则返回 IRQ_NONE（不是本设备的中断），为 1 则处理并返回 IRQ_HANDLED。所有共享者的 handler 按注册顺序依次调用。如果某个 handler 总是返回 IRQ_NONE，会被 spurious IRQ 检测禁用。"),
     ("模块卸载时为什么要先 free_irq()？如果忘记会怎样？",
      "free_irq() 释放 IRQ 线并注销 handler。如果忘记：① 硬件触发中断时 handler 已被卸载 → 空指针 dereference → kernel panic。② IRQ 持续触发无人处理 → 中断风暴 → CPU 100% 在中断处理。正确做法：模块 exit 函数中 `free_irq(irq, dev_id)` 先注销，再释放其他资源。")]
)

NOTES[f"{C7}/section-7.5-中断上下文.md"] = (
    ["混淆中断上下文和进程上下文——中断上下文无 task_struct、不可睡眠、不可调度",
     "以为在中断上下文中可以调用任何内核函数——不能调用会睡眠的函数（mutex/kmalloc(GFP_KERNEL)）",
     "忽略 preempt_count 的作用——preempt_count 跟踪当前是否在中断/softirq/持锁上下文"],
    [("中断上下文和进程上下文的本质区别？",
      "进程上下文：有 task_struct（current 有效）、有进程栈、可调度/可睡眠。中断上下文：无 task_struct（current 是被中断的进程但不应使用）、用中断栈、不可调度/不可睡眠。判断：`in_interrupt()` 返回 true = 中断上下文。`in_irq()` = hard IRQ，`in_softirq()` = softirq。"),
     ("preempt_count 的各字段含义？",
      "`preempt_count` 是 32 位计数器：[0-7] preempt（抢占计数）、[8-11] softirq、[12-15] hardirq、[16-19] NMI、[20-23] RCU read。每进一层 hard IRQ，hardirq 位 +1。`preempt_count() == 0` 时才允许 schedule()。HFT 可用 `bpftrace` 追踪 `preempt_count` 变化。"),
     ("HFT 用户态如何避免进入中断上下文？",
      "用户态本身不进入中断上下文，但中断会抢占用户线程的 CPU。避免方法：① `isolcpus` 隔离交易核，中断路由到其他核。② `nohz_full` 停止定时器中断。③ DPDK 绕过内核网络栈。④ `SCHED_FIFO` 确保中断返回后立即恢复交易线程。")]
)

NOTES[f"{C7}/section-7.6-中断处理机制的实现.md"] = (
    ["把 ULK 的 do_IRQ() 当现代版——6.x 的 IRQ 入口用 IDTENTRY 宏 + IRQ domain 树",
     "混淆硬件 IRQ 号和 Linux virtual IRQ（virq）——硬件号可能冲突，virq 全局唯一",
     "以为 /proc/interrupts 显示的是硬件 IRQ 号——显示的是 virq"],
    [("现代内核的 IRQ domain 机制解决了什么问题？",
      "ULK 时代 IRQ 号 = 硬件中断号，全局数组 irq_desc[] 直接索引。现代内核支持中断控制器级联（GIC → GPIO → MSI），硬件号可能冲突。IRQ domain 为每级中断控制器建立独立的号码空间，通过 irq_domain_translate() 将硬件号映射为唯一的 Linux virtual IRQ（virq）。/proc/interrupts 显示的是 virq。"),
     ("do_IRQ() 的现代实现和 ULK 时代有什么区别？",
      "ULK：全局 irq_desc[] 数组直接索引。现代：① per-CPU `vector_irq[]` 映射 CPU vector → irq_desc。② IRQ domain 树处理级联控制器。③ `handle_irq()` 调用 `generic_handle_irq_desc()` → `__handle_irq_event_percpu()` 遍历 action 链。④ x86-64 用 IDTENTRY 宏自动生成入口 stub。"),
     ("HFT 如何查看和调整中断分布？",
      "① `cat /proc/interrupts`：各 CPU 的中断计数。② `cat /proc/irq/[n]/smp_affinity_list`：IRQ 绑定的 CPU。③ `echo 0,1 > /proc/irq/[n]/smp_affinity_list`：绑中断到 CPU 0 和 1。④ `watch -n1 grep . /proc/interrupts`：实时监控。⑤ `ethtool -x eth0`：查看 RSS 中断分布。HFT 确保 NIC 中断不到交易核。")]
)

NOTES[f"{C7}/section-7.7-中断控制.md"] = (
    ["混淆 local_irq_disable() 和 local_irq_save()——前者不保存状态，后者保存",
     "以为 local_irq_disable() 禁用所有 CPU 的中断——只禁本地 CPU",
     "在 spin_lock_irqsave() 后手动 local_irq_enable()——会破坏锁的 IRQ 保护"],
    [("local_irq_disable() 和 local_irq_save(flags) 的区别？什么时候用哪个？",
      "local_irq_disable()：无条件关中断，不保存之前状态。如果你不知道调用前中断是否已关，用这个可能破坏调用者的状态。local_irq_save(flags)：保存 RFLAGS.IF 到 flags 再关中断。local_irq_restore(flags) 恢复。内核代码应始终用 _save/_restore 版本。"),
     ("spin_lock() / spin_lock_irq() / spin_lock_irqsave() / spin_lock_bh() 的选择？",
      "spin_lock()：进程上下文 + 短临界区。spin_lock_irq()：知道中断当前开着时用。spin_lock_irqsave(flags)：最安全，保存+关中断（推荐）。spin_lock_bh()：禁 softirq 但不禁 hard IRQ。选择原则：如果中断也访问同一锁 → spin_lock_irqsave。如果只有 softirq 访问 → spin_lock_bh。进程上下文独享 → spin_lock。"),
     ("HFT 如何减少中断控制的副作用？",
      "① 避免 `local_irq_disable()` 在用户态（用户态不能关中断）。② 用 `isolcpus` + 中断重定向代替手动关中断。③ `SCHED_FIFO` + 绑核让交易线程不受中断影响。④ 如果必须在内核模块中关中断，临界区 <1us（否则影响系统响应）。⑤ `preempt_disable()` 比 `local_irq_disable()` 开销小，优先考虑。")]
)

# === Ch8 Bottom Halves (8 notes) ===
C8 = "chapter-08-bottom-halves/notes"
NOTES[f"{C8}/section-8.1-下半部概念与必要性.md"] = (
    ["以为中断处理函数可以做所有工作——耗时操作必须延迟到下半部，否则中断延迟过大",
     "混淆上半部和下半部的执行上下文——上半部在 hard IRQ 上下文，下半部在 softirq/进程上下文",
     "在下半部中用错误的同步原语——softirq/tasklet 只能用 spinlock，workqueue 可以用 mutex"],
    [("为什么需要下半部机制？不能全在中断处理函数中做吗？",
      "中断处理函数运行在中断上下文，期间该 CPU 的中断可能被禁用（或同优先级中断被阻塞）。如果处理时间长：① 其他中断延迟增大。② 系统响应变慢。③ 可能丢中断。解决方案：上半部只做最紧急的（确认硬件 + 读取数据），大部分工作延迟到下半部（softirq/tasklet/workqueue）执行。"),
     ("上半部和下半部的执行上下文有什么区别？",
      "上半部：hard IRQ 上下文，无 task_struct，不可睡眠，不可调度，中断可能被禁用。下半部：softirq/tasklet 仍在 softirq 上下文（不可睡眠但可被中断抢占），workqueue 在 kworker 线程（进程上下文，可睡眠/可调度）。HFT 热路径应避免在 softirq 中做大量工作。"),
     ("HFT 中下半部机制对延迟的影响？",
      "softirq 在 hard IRQ 返回时执行，会延迟用户线程恢复。NET_RX_SOFTIRQ（收包）是最常见的延迟源。解决：① RPS/RFS 把 softirq 迁移到非交易核。② NAPI 轮询减少 softirq 频率。③ DPDK 完全绕过 softirq。④ `nohz_full` 减少定时器 softirq。")]
)

NOTES[f"{C8}/section-8.2-下半部机制的历史与演进.md"] = (
    ["把 BH（Bottom Half）当现代机制——BH 在 2.5 已删除，被 softirq/tasklet/workqueue 取代",
     "以为 tasklet 是新机制——tasklet 基于 softirq，且正在被废弃",
     "混淆 workqueue 的旧版（keventd）和现代版（cmwq）——现代 workqueue 是并发可配置的"],
    [("下半部机制的历史演进？哪些已被删除？",
      "① BH（Bottom Half）：2.0-2.4，全局串行（同一时间只有一个 BH 执行），2.5 删除。② Task Queue：2.0-2.5，复杂且不灵活，2.5 删除。③ softirq：2.3 引入，仍存在。④ tasklet：2.3 引入，基于 softirq，正在被废弃。⑤ workqueue：2.5 引入，2.6.36 改为 cmwq（Concurrency Managed Workqueue），仍存在且推荐。"),
     ("tasklet 为什么被废弃？推荐用什么替代？",
      "tasklet 缺陷：① 同类型全局串行化（不能多 CPU 并发），性能差。② 基于 softirq 不能睡眠。③ API 复杂。替代：需要并发 → workqueue。需要低延迟 → threaded IRQ。需要定时回调 → hrtimer。内核已标记 tasklet 为 deprecated，新代码不应使用。"),
     ("现代 workqueue（cmwq）和旧版有什么区别？",
      "旧版（2.5-2.6.35）：每 CPU 一个 worker 线程（`events/n`），如果 work 阻塞会卡住该 CPU 所有 work。cmwq（2.6.36+）：动态创建/销毁 worker 线程，blocked worker 时自动 spawn 新的。`alloc_workqueue()` 可配置 `WQ_UNBOUND`（不绑 CPU）、`WQ_HIGHPRI`（高优先级）、`max_active`（最大并发）。")]
)

NOTES[f"{C8}/section-8.3-软中断.md"] = (
    ["在 softirq 中调用睡眠函数——softirq 不能 schedule()，只能用 spinlock",
     "以为 softirq 可以动态注册——softirq 是编译时静态注册的（10 种类型），不能运行时添加",
     "混淆 softirq 的执行时机——在 hard IRQ 返回时 + ksoftirqd 内核线程中执行"],
    [("softirq 的 10 种类型中哪些和 HFT 相关？",
      "HI_SOFTIRQ：高优先级 softirq（tasklet_hi 用）。TIMER_SOFTIRQ：定时器到期。NET_TX_SOFTIRQ / NET_RX_SOFTIRQ：网络收发（HFT 最关注）。BLOCK_SOFTIRQ：块 I/O 完成。TASKLET_SOFTIRQ：tasklet。SCHED_SOFTIRQ：负载均衡。HRTIMER_SOFTIRQ：高精度定时器。RCU_SOFTIRQ：RCU 回收。HFT 主要受 NET_RX 和 TIMER 影响。"),
     ("softirq 的执行时机和机制？",
      "① hard IRQ 返回时：`__do_softirq()` 检查 pending softirq，执行最多 10 个 + 2ms 时间限制。② ksoftirqd：如果有持续 pending 的 softirq，唤醒 ksoftirqd 内核线程处理。③ 显式触发：`raise_softirq(NR_SOFTIRQ)`。softirq 在 per-CPU 上执行，同类型不会在多个 CPU 上并发。"),
     ("HFT 如何减少 softirq 对交易线程的影响？",
      "① RPS（Receive Packet Steering）：`/sys/class/net/eth0/queues/rx-0/rps_cpus` 把收包 softirq 迁移到非交易核。② NAPI：网卡在中断后切换到轮询模式，减少中断+softirq 频率。③ `nohz_full`：隔离交易核的 softirq。④ DPDK：完全绕过内核网络栈，用户态轮询。⑤ `cat /proc/softirqs` 监控 softirq 频率。")]
)

NOTES[f"{C8}/section-8.4-tasklet.md"] = (
    ["在 6.x 内核中还在用 tasklet——tasklet 已 deprecated，推荐 workqueue/threaded IRQ",
     "混淆 tasklet 和 softirq——tasklet 基于 softirq（TASKLET_SOFTIRQ/HI_SOFTIRQ），是 softirq 的封装",
     "以为同类型 tasklet 可以多 CPU 并发——不行，同类型 tasklet 全局串行化"],
    [("tasklet 的核心特征和限制？",
      "特征：① 基于 softirq（动态注册，不像 softirq 需编译时注册）。② 同类型 tasklet 不会在多 CPU 上并发执行（全局串行化）。③ 在 softirq 上下文运行，不能睡眠。限制：① 串行化导致性能差（不能利用多核）。② 不能睡眠/持 mutex。③ 已 deprecated。替代方案：workqueue（可并发可睡眠）或 threaded IRQ。"),
     ("为什么 tasklet 要被废弃？用什么替代？",
      "① 同类型串行化：多核系统上性能瓶颈。② 不能睡眠：限制了使用场景。③ API 复杂且容易误用。替代：需要并发 → workqueue（alloc_workqueue + queue_work）。需要低延迟 + 可睡眠 → threaded IRQ（request_threaded_irq）。需要定时 → hrtimer。内核社区计划在 future version 移除 tasklet API。"),
     ("如果遇到旧代码中的 tasklet，怎么迁移到 workqueue？",
      "```c\n// 旧: tasklet\nDECLARE_TASKLET(my_tasklet, my_func, data);\ntasklet_schedule(&my_tasklet);\n// 新: workqueue\nstruct work_struct my_work;\nINIT_WORK(&my_work, my_func);\nschedule_work(&my_work);\n// 区别: work 可以睡眠/持mutex, 可多CPU并发\n```")]
)

NOTES[f"{C8}/section-8.5-工作队列.md"] = (
    ["在 workqueue 中以为不能睡眠——workqueue 运行在 kworker 线程（进程上下文），可以睡眠",
     "混淆 system workqueue 和自定义 workqueue——system workqueue 共享，自定义更可控",
     "在热路径上用 workqueue——workqueue 有线程唤醒 + 调度延迟，不适合纳秒级热路径"],
    [("workqueue 为什么可以睡眠？它的执行上下文是什么？",
      "workqueue 的 work 函数在 kworker 内核线程中执行（进程上下文）。kworker 有 task_struct、有进程栈、可调度。所以 work 函数可以 mutex_lock()、kmalloc(GFP_KERNEL)、甚至 msleep()。这是 workqueue 相比 softirq/tasklet 的最大优势。代价：唤醒 + 调度延迟（~1-5us）。"),
     ("system workqueue（schedule_work）和自定义 workqueue（alloc_workqueue）的区别？",
      "system workqueue（`system_wq`）：全局共享，所有 `schedule_work()` 提交的 work 共享 kworker 池。优点：简单，无需管理。缺点：可能被其他模块的 work 阻塞。自定义 workqueue：`alloc_workqueue(name, flags, max_active)`，独立 kworker 池。flags: WQ_UNBOUND（不绑 CPU）、WQ_HIGHPRI（高优先级）。驱动推荐用自定义 workqueue。"),
     ("HFT 什么时候该用 workqueue？什么时候不该？",
      "该用：驱动初始化（等硬件就绪）、配置变更（可睡眠）、错误处理（需 I/O）。不该用：热路径数据收发（延迟太大）、纳秒级操作（用 softirq 或直接在 hard IRQ 中处理）。HFT 设计原则：热路径 → 无锁 + 预分配 + 用户态轮询。非热路径 → workqueue 处理异步任务。")]
)

NOTES[f"{C8}/section-8.6-ksoftirqd-辅助线程.md"] = (
    ["以为 softirq 只在 hard IRQ 返回时执行——ksoftirqd 内核线程也会处理积压的 softirq",
     "混淆 softirq 在 IRQ 返回和 ksoftirqd 中的执行优先级——ksoftirqd 是普通优先级线程，可能被 RT 抢占",
     "以为 ksoftirqd 是 per-IRQ 的——ksoftirqd 是 per-CPU 的（每 CPU 一个 ksoftirqd/n）"],
    [("ksoftirqd 的作用和触发条件？",
      "每 CPU 一个 ksoftirqd/n 内核线程（nice=0，普通优先级）。触发条件：softirq 积压——`__do_softirq()` 在 hard IRQ 返回时执行后，如果还有 pending softirq（超过 10 次循环或 2ms 限制），唤醒 ksoftirqd 继续处理。ksoftirqd 在进程上下文运行（可被调度/抢占），但仍在 softirq 上下文（不能睡眠）。"),
     ("ksoftirqd 和 hard IRQ 返回时的 softirq 执行有什么区别？",
      "hard IRQ 返回时：softirq 在中断上下文执行，优先级高，可能延迟用户线程。ksoftirqd：softirq 在 kworker 线程上下文执行，优先级低（nice=0），可被 RT/CFS 高优先级任务抢占。如果 softirq 频率太高，hard IRQ 返回路径会主动 defer 到 ksoftirqd，避免在中断上下文耗时过长。"),
     ("HFT 如何避免 ksoftirqd 干扰交易核？",
      "① `isolcpus=N` + `nohz_full=N`：N 号核上 ksoftirqd 几乎不被唤醒（无 softirq 积压）。② RPS/RFS：网络 softirq 迁移到其他核。③ `ps -eo pid,comm,psr | grep ksoftirqd`：确认 ksoftirqd 不在交易核上运行。④ DPDK：绕过 softirq。⑤ `cat /proc/[ksoftirqd_pid]/stat`：检查 ksoftirqd 运行时间。")]
)

NOTES[f"{C8}/section-8.7-如何选择下半部机制.md"] = (
    ["在所有场景都用 workqueue——workqueue 有调度延迟，不适合对延迟敏感的场景",
     "以为 softirq 总是最快的——softirq 在 IRQ 返回时执行，可能延迟用户线程恢复",
     "忽略 threaded IRQ——threaded IRQ 是 modern 替代 tasklet 的好方案"],
    [("给定场景如何选择下半部机制？",
      "需要睡眠/持 mutex/I/O → workqueue。需要低延迟 + 不睡眠 → softirq。需要低延迟 + 可睡眠 → threaded IRQ。定时回调 → hrtimer（not softirq TIMER_SOFTIRQ）。网络收发 → NAPI（基于 softirq 的轮询）。不要用 → tasklet（deprecated）。关键考量：延迟（softirq < threaded IRQ < workqueue）vs 灵活性（workqueue > threaded IRQ > softirq）。"),
     ("threaded IRQ 相比 softirq/tasklet 有什么优势？",
      "① 可睡眠：thread_fn 在内核线程中运行，可 mutex_lock/kmalloc(GFP_KERNEL)。② RT 友好：threaded IRQ 线程有优先级，RT 内核中可设高优先级。③ 可调试：线程有 /proc/[pid]/ 可追踪。④ 简化锁：hard IRQ 只确认硬件（持 spinlock 短时间），thread_fn 可持 mutex（长时间）。缺点：唤醒+调度延迟（~1-5us）。"),
     ("HFT 系统中下半部机制的最佳实践？",
      "① 热路径（收发行情/下单）：DPDK 用户态轮询，完全绕过 softirq/workqueue。② 非热路径（配置/监控）：workqueue 处理，不影响交易核。③ 内核模块中断：request_threaded_irq()，hard IRQ 只 ACK，thread_fn 做处理。④ 定时器：hrtimer 代替 TIMER_SOFTIRQ（精度更高）。⑤ 确保 ksoftirqd 不在交易核运行。")]
)

NOTES[f"{C8}/section-8.8-锁定与禁用下半部.md"] = (
    ["混淆 spin_lock_bh() 和 local_bh_disable()——前者锁+禁 softirq，后者只禁 softirq",
     "在 spin_lock_bh() 后手动 local_bh_enable()——会破坏锁保护",
     "以为 local_bh_disable() 禁用了 hard IRQ——只禁 softirq，hard IRQ 仍可触发"],
    [("spin_lock_bh() 和 local_bh_disable() 的区别？",
      "spin_lock_bh(lock)：① 获取 spinlock。② 禁用本地 softirq（递增 preempt_count 的 softirq 位）。用于保护 softirq 和进程上下文都访问的数据。local_bh_disable()：只禁 softirq，不锁。用于 softirq 临界区不需要锁但需要禁 softirq 的场景（如 per-CPU 统计）。对应的恢复：spin_unlock_bh() / local_bh_enable()。"),
     ("local_bh_disable() 禁用 softirq 后，hard IRQ 还能触发吗？",
      "能。local_bh_disable() 只递增 preempt_count 的 softirq 位（SOFTIRQ_OFFSET），不影响 hard IRQ。hard IRQ 仍可触发和执行。如果需要同时禁 hard IRQ 和 softirq，用 local_irq_disable()。如果需要禁 softirq 但允许 hard IRQ，用 local_bh_disable()。"),
     ("HFT 为什么需要关心 softirq 禁用？",
      "HFT 内核模块（如定制 NIC 驱动）可能需要在 softirq 上下文操作共享数据。spin_lock_bh() 确保 softirq 不会在持锁期间重入。但 HFT 用户态通常不直接处理 softirq——通过 `isolcpus` + 中断重定向 + RPS/RFS 把 softirq 推到非交易核。测量：`perf stat -e softirq` 统计 softirq 频率。")]
)

# === Ch9 Kernel Sync Intro (6 notes) ===
C9 = "chapter-09-kernel-sync-intro/notes"
NOTES[f"{C9}/section-9.1-临界区与竞态条件.md"] = (
    ["以为单核不需要同步——UP 上仍需禁抢占/中断防止竞态",
     "混淆「竞态条件」和「数据竞争」——竞态是逻辑层（结果依赖时序），数据竞争是内存层（并发读写同一地址）",
     "以为原子操作能解决所有竞态——原子操作只保证单操作原子性，多操作组合仍需锁"],
    [("什么是临界区？为什么需要保护？",
      "临界区是访问共享资源的代码段。如果不保护：多 CPU/中断/抢占并发执行 → 数据竞争 → 数据损坏/崩溃。例：`counter++` 实际是 load → add → store 三步，两 CPU 同时执行可能丢失一次更新。保护方式：spinlock/mutex/atomic/RCU，确保同一时间只有一个执行者进入临界区。"),
     ("UP（单处理器）上为什么还需要同步？",
      "UP 上没有多 CPU 并行，但有：① 抢占：进程在临界区中被抢占，另一进程进入临界区。② 中断：进程在临界区中被中断，中断处理函数访问同一数据。解决：进程上下文 → preempt_disable() 或 spinlock（UP 上退化为 preempt_disable）。中断也访问 → spin_lock_irqsave()。"),
     ("HFT 用户态的竞态条件和内核有什么不同？",
      "用户态竞态来源：多线程 + 信号 + atexit handlers。用户态不能用内核锁，用：① `std::atomic`（无锁，适合简单操作）。② `std::mutex`/`pthread_mutex`（基于 futex，适合复杂临界区）。③ 无锁数据结构（SPSC 队列等）。HFT 优先无锁设计避免 futex 的 syscall 开销。ThreadSanitizer 可检测数据竞争。")]
)

NOTES[f"{C9}/section-9.2-加锁.md"] = (
    ["以为锁只是「加锁/解锁」——还需要考虑锁粒度、锁顺序、死锁预防",
     "把锁粒度越细越好——太细导致锁开销 > 并发收益，太粗导致并行度低",
     "在持锁时做 I/O 或睡眠——spinlock 不能睡眠，mutex 可以但会降低并发度"],
    [("锁粒度怎么选择？太粗和太细各有什么问题？",
      "太粗（一把大锁保护整个数据结构）：① 并行度低（多线程串行化）。② 锁竞争严重。太细（每个字段一把锁）：① 锁开销 > 并发收益。② 锁管理复杂。③ 死锁风险高。原则：① 热路径用细粒度锁（per-CPU/per-bucket）。② 冷路径用粗粒度锁（简单不易错）。③ 先粗后细：先用一把大锁保证正确，prof 后细化热点。"),
     ("spinlock 和 mutex 什么时候用？持锁时能做什么不能做什么？",
      "spinlock：短临界区（<1us），不能睡眠/调度。适合：中断上下文、per-CPU 数据、简单计数器。mutex：长临界区（>1us），可睡眠/调度/做 I/O。适合：复杂数据结构遍历、文件操作、内存分配。选择标准：持锁时间 < 上下文切换时间（~1-5us）→ spinlock；> → mutex。"),
     ("HFT 中锁选择有什么特殊考量？",
      "① 热路径避免锁——用无锁数据结构（SPSC/MPMC 队列）。② 必须用锁时优先 spinlock（用户态 `std::atomic_flag` 自旋）——避免 futex syscall。③ `std::mutex` 底层是 futex，竞争时进内核 → 微秒级延迟。④ RT 优先级继承：`PTHREAD_PRIO_INHERIT` 防优先级反转。⑤ `perf lock` 分析锁持有时间和竞争。")]
)

NOTES[f"{C9}/section-9.3-并发的原因.md"] = (
    ["以为只有多 CPU 才有并发——中断、抢占、softirq 都是并发来源",
     "忽略中断引起的并发——进程在修改数据时被中断，中断处理函数也修改同一数据",
     "以为 preempt_disable() 能防止所有并发——只防抢占，不防中断"],
    [("内核中并发的来源有哪些？",
      "① SMP：多 CPU 同时执行。② 中断：hard IRQ 打断进程/softirq。③ softirq：softirq 打断进程上下文。④ 抢占：CONFIG_PREEMPT 时进程可被另一进程抢占。⑤ 信号：某些操作可被信号中断。⑥ 线程化：同一进程的多个内核线程。分析并发：问「这段代码是否可能被另一个执行路径同时进入？」如果是 → 需要同步。"),
     ("preempt_disable() 能防止所有并发吗？",
      "不能。preempt_disable() 只防止本 CPU 上的抢占（内核态），不防：① 其他 CPU 上的并行执行（SMP）。② 中断（hard IRQ 仍可触发）。③ softirq（仍可执行）。全面防止：preempt_disable() + local_irq_disable() + spinlock（防 SMP）。或直接 spin_lock_irqsave()（一步到位）。"),
     ("HFT 用户态的并发来源和内核有什么不同？",
      "用户态并发来源：① 多线程（pthread）。② 信号处理函数。③ atexit/fork handlers。④ 多进程共享内存。用户态不能禁中断/禁抢占（需要 root + 内核模块），所以用：① `std::atomic` 无锁同步。② `std::mutex`（futex）。③ `pthread_cancel` 屏蔽。④ 共享内存 + 原子操作。HFT 热路径只用无锁设计。")]
)

NOTES[f"{C9}/section-9.4-知道要保护什么.md"] = (
    ["以为只需要保护全局变量——局部变量也可能被中断/信号修改（如 static 局部变量）",
     "忽略 per-CPU 变量的同步——per-CPU 变量在同一 CPU 上仍需禁中断/softirq 保护",
     "混淆「共享」的范围——同一进程的多个线程共享全局变量，不同进程通过共享内存共享"],
    [("什么数据需要同步保护？判断标准是什么？",
      "标准：① 多个执行路径可能同时访问。② 至少一个路径是写操作。满足两条 → 需要保护。典型：全局变量、共享数据结构（链表/树/哈希表）、硬件寄存器、文件描述符表。不需要：局部变量（栈上，每个执行路径独立）、per-CPU 变量（无跨 CPU 访问时）。但 per-CPU 变量在同 CPU 上仍需防中断/softirq。"),
     ("per-CPU 变量为什么也需要同步？",
      "per-CPU 变量在不同 CPU 上是独立的（无 SMP 竞态），但同一 CPU 上：① 进程在写 per-CPU 变量 → 被中断 → 中断处理函数也写该变量 → 竞态。解决：`this_cpu_inc()`（原子 per-CPU 操作）或 `local_irq_save()` + `this_cpu_write()`。`get_cpu_var()` 自动禁抢占。"),
     ("HFT 中如何设计无共享（share-nothing）架构避免同步？",
      "① 每个交易线程有独立的数据副本（per-thread 变量）。② 线程间通过无锁消息队列通信（SPSC）。③ 共享只读数据（行情快照）用 `mmap(MAP_SHARED)` + COW。④ 统计数据用 per-thread 计数器，定期聚合。⑤ 避免 `std::shared_ptr`（原子引用计数有开销），用 `unique_ptr` + 明确所有权。")]
)

NOTES[f"{C9}/section-9.5-死锁.md"] = (
    ["以为死锁只会发生在多锁场景——单锁也能死锁（如递归加锁同一 spinlock）",
     "混淆死锁的四种条件——互斥、持有并等待、不可剥夺、循环等待",
     "忽略 lockdep——lockdep 能在开发阶段检测潜在死锁，生产阶段关掉"],
    [("死锁的四个必要条件？",
      "① 互斥：资源同一时间只能被一个执行者使用。② 持有并等待：持有资源的执行者可以请求新资源。③ 不可剥夺：资源不能被强制夺走（只能持有者主动释放）。④ 循环等待：存在执行者的循环等待链。打破任一条件即可预防死锁。最常用：打破循环等待——规定锁的全局获取顺序。"),
     ("内核中常见的死锁场景？",
      "① 递归加锁：同一 spinlock 在持锁期间再次 lock → 自死锁。② AB-BA 死锁：线程1 lock(A)→lock(B)，线程2 lock(B)→lock(A)。③ 中断死锁：进程持 lock()，被中断，中断处理函数也 lock() → 死锁。解决：中断访问的锁用 spin_lock_irqsave()。④ softirq 死锁：用 spin_lock_bh()。预防：lockdep + 全局锁顺序。"),
     ("lockdep 怎么使用？能检测什么？",
      "`CONFIG_LOCKDEP=y` 编译内核。开启：`echo 1 > /proc/sys/kernel/lock_stat`。检测：① AB-BA 死锁（锁顺序反转）。② 递归加锁。③ 中断上下文持有可睡眠锁。④ IRQ 安全性不匹配。开销：~10% 性能下降，仅开发阶段开启。用户态类似工具：ThreadSanitizer (`-fsanitize=thread`)。HFT 开发应 CI 中跑 lockdep/TSan。")]
)

NOTES[f"{C9}/section-9.6-争用和可扩展性.md"] = (
    ["以为锁的正确性就够了——锁的争用程度直接影响性能和可扩展性",
     "混淆锁持有时间和锁等待时间——持有时间是「锁住了多久」，等待时间是「等了多久才拿到」",
     "以为增加 CPU 数量总能提升性能——锁争用下，增加 CPU 反而降低吞吐（锁竞争恶化）"],
    [("锁争用（contention）怎么测量？",
      "内核：① `perf lock record` + `perf lock report`：记录锁等待时间和持有时间。② `/proc/lock_stat`：lockdep 统计。③ `bpftrace -e 'tracepoint:lock:lock_acquire { ... }'`：追踪锁获取。用户态：① `perf lock`。② `Valgrind --tool=drd`。③ `pthread_mutex` 的 `trylock` 探测。指标：con-<N>（等待次数）、wait-total（总等待时间）、hold-total（总持有时间）。"),
     ("为什么增加 CPU 在锁争用下反而降低性能？",
      "假设一把全局锁保护共享数据。N 个 CPU 同时请求锁：① 只有 1 个拿到，其余 N-1 个 spin 等待。② N 越大，spin 浪费的 CPU 越多。③ 锁释放时 N-1 个 CPU 抢锁 → cache line bouncing。④ 吞吐随 N 增加先升后降（Amdahl's Law 的锁版本）。解决：① 减小临界区。② per-CPU 数据。③ 无锁数据结构（RCU）。"),
     ("HFT 如何设计无锁/低争用数据结构？",
      "① SPSC 环形队列：单生产者单消费者，`atomic<head>` + `atomic<tail>` + release/acquire 序。② per-thread 缓存：每线程独立操作，定期聚合。③ RCU 模式：读端无锁（`atomic load` 指针），写端复制+替换+延迟回收。④ 分片锁：`sharded_hashmap`，N 个 bucket 各一把锁，减少争用。⑤ `std::shared_mutex`：多读单写，适合读多写少。")]
)

# === Ch10 Sync Methods (11 notes) ===
C10 = "chapter-10-sync-methods/notes"
NOTES[f"{C10}/section-10.1-原子操作.md"] = (
    ["把原子操作当万能锁——原子操作只保证单操作原子性，多操作组合仍需锁",
     "混淆 atomic_t 和 refcount_t——refcount_t 防溢出（0 不会变 -1），atomic_t 会",
     "以为 atomic_read() 是原子操作——在 x86 上它只是普通读（volatile），不保证其他 CPU 的写入可见"],
    [("atomic_t 和 refcount_t 的区别？",
      "atomic_t：纯原子计数器，`atomic_dec(&v)` 可以从 0 变成 -1（UAF 漏洞）。refcount_t：引用计数专用，`refcount_dec()` 在 0 时 WARN + 阻止下溢。6.x 内核中 task_struct 的 usage 已从 atomic_t 改为 refcount_t。安全代码应始终用 refcount_t 管理生命周期。"),
     ("`atomic_inc(&v)` 在 x86-64 上实际生成什么指令？",
      "`lock incl (%rdi)`——LOCK 前缀 + incl 指令。LOCK 前缀锁 cache line（通过 MESI 协议的 Read-Modify-Write 周期），确保原子性。开销：~20-40 cycles（无争用时）。争用时 cache line bouncing，可达数百 cycles。ARM64 上生成 `ldaxr`/`stlxr`（独占加载/存储）循环。"),
     ("HFT 用户态如何高效使用原子操作？",
      "```c\n// 无锁 SPSC 队列\nstd::atomic<size_t> head{0}, tail{0};\n// 生产者\nhead.store(head.load(std::memory_order_relaxed) + 1,\n           std::memory_order_release);\n// 消费者\nsize_t h = head.load(std::memory_order_acquire);\nif (h > tail.load(std::memory_order_relaxed)) {\n    // 有数据\n}\n// 关键: release/acquire 配对, 避免 seq_cst 的全屏障开销\n```")]
)

NOTES[f"{C10}/section-10.2-自旋锁.md"] = (
    ["在持 spinlock 时调用睡眠函数——会 BUG: scheduling while atomic panic",
     "混淆 spin_lock() 和 spin_lock_irqsave()——前者不关中断，后者保存+关",
     "以为 spinlock 有公平性保证——spinlock 不保证公平，可能饿死某些等待者"],
    [("spin_lock() / spin_lock_irqsave() / spin_lock_bh() 什么时候用哪个？",
      "spin_lock()：进程上下文 + 确认无中断/softirq 访问同一锁。spin_lock_irqsave(flags)：中断也可能访问同一锁。spin_lock_bh()：softirq 可能访问但 hard IRQ 不会。如果不确定 → 用 spin_lock_irqsave()（最安全）。UP 上 spin_lock() 退化为 preempt_disable()，spin_lock_irqsave() 退化为 local_irq_save()。"),
     ("持 spinlock 时为什么不能睡眠？",
      "Spinlock 假设等待者会忙等（spin）。如果持锁者睡眠（schedule()）：① 等待者无限 spin 浪费 CPU。② schedule() 检查 preempt_count > 0 → BUG: scheduling while atomic → panic。③ 如果切换到的进程也请求同一锁 → 死锁。RT 内核把 spinlock 变成可睡眠锁后这个限制不成立。"),
     ("HFT 用户态的 spinlock 和内核有什么不同？",
      "用户态没有真正的 spinlock（不关中断/不禁抢占）。`std::atomic_flag` + test_and_set 自旋是最接近的。区别：① 用户态 spinlock 被调度器抢占后仍 spin（浪费 CPU 时间片）。② 不能关中断防止抢占。③ `sched_yield()` 可以让出 CPU 但不释放锁。HFT 用户态 spinlock 应限制在 <100ns 临界区，超时改用 futex/mutex。")]
)

NOTES[f"{C10}/section-10.3-读-写自旋锁.md"] = (
    ["以为读写锁总是比普通锁好——写多读少时读写锁退化（写者饥饿），反而更慢",
     "混淆 rwlock 和 RCU——rwlock 读端仍需原子操作（有开销），RCU 读端无开销",
     "在读写锁的读端做耗时操作——会阻塞写者（写者等所有读者退出）"],
    [("读写自旋锁（rwlock）适合什么场景？有什么缺点？",
      "适合：读多写少（如路由表/配置表），多个读者可并发。缺点：① 写者饥饿：如果有持续的新读者进来，写者可能无限等待。② 读端仍有原子操作开销（递增 reader count）。③ 公平性：部分实现有公平性保证（防止写者饥饿），但会降低读吞吐。现代内核更推荐 RCU（读端零开销）替代 rwlock。"),
     ("RCU 相比 rwlock 有什么优势？",
      "RCU 读端：`rcu_read_lock()` 只禁抢占（无原子操作，零开销）。rwlock 读端：`read_lock()` 原子递增 reader count（有 cache line bouncing）。RCU 写端：复制 + 替换指针 + 等 grace period。rwlock 写端：等所有读者退出。RCU 适合：读极多写极少。rwlock 适合：读写都有但读多。RCU 缺点：写端延迟大（等 grace period）。"),
     ("HFT 中如何选择读写锁 vs RCU vs 无锁？",
      "① 读极多写极少（路由表/配置）：RCU（内核）/ `std::shared_ptr<const T>`（用户态）。② 读写都有但读多：`std::shared_mutex`（用户态）/ rwlock（内核）。③ 热路径数据：无锁（SPSC 队列/per-thread 数据）。④ 配置变更：双缓冲（atomic swap pointer + 延迟释放旧版）。HFT 原则：热路径零开销，冷路径可接受锁。")]
)

NOTES[f"{C10}/section-10.4-信号量.md"] = (
    ["混淆 semaphore 和 mutex——mutex 有归属（只有持有者能解锁），semaphore 无归属",
     "以为 counting semaphore 常用于内核——内核中大多用 mutex，semaphore 只在特殊场景",
     "在中断上下文中调用 down()——down() 会睡眠，中断只能用 down_trylock()"],
    [("semaphore 和 mutex 的核心区别？",
      "mutex：① 有归属（owner 字段），只有持锁者能 unlock。② 支持优先级继承（rt_mutex）。③ 支持 lockdep。semaphore：① 无归属，任何人可以 up()。② 初始值可 >1（计数信号量）。③ 无优先级继承。内核新代码推荐 mutex，semaphore 只在需要计数语义或无归属场景使用。"),
     ("counting semaphore 在什么场景下有用？",
      "① 限制并发数：如限制同时打开的文件数（初始值=最大并发数）。② 生产者-消费者：semaphore 计数 = 队列中可用元素数，消费者 down() 取数据，生产者 up() 放数据。③ 资源池：初始值=池大小，获取资源 down()，释放 up()。但在内核中，这些场景更常用 kfifo + waitqueue 或 mempool。"),
     ("HFT 中 semaphore 的用户态对应物？",
      "① `std::counting_semaphore<N>`（C++20）：计数信号量。② `sem_t`（POSIX）：进程间或线程间信号量。③ `std::binary_semaphore` = mutex 的近似。HFT 热路径避免 semaphore（有 futex 开销），用无锁队列代替生产者-消费者模式。非热路径可以用 semaphore 做资源限流。")]
)

NOTES[f"{C10}/section-10.5-互斥体.md"] = (
    ["在 mutex 和 semaphore 之间纠结——内核新代码始终优先 mutex",
     "以为 mutex_lock() 一定睡眠——无争用时直接原子获取（fast path），不进内核",
     "忽略优先级继承——mutex 默认不支持，需要 rt_mutex"],
    [("mutex 的 fast path / slow path 是什么？",
      "Fast path（无争用）：`mutex_lock()` → 原子 CAS 将 owner 从 NULL 设为 current → 成功返回。开销 ~20ns。Slow path（有争用）：CAS 失败 → `__mutex_lock_slowpath()` → 加入等待队列 → `schedule()` 睡眠 → 被唤醒后重试 CAS。开销 ~1-5us。大部分场景 fast path 命中，mutex 性能接近 spinlock。"),
     ("mutex 和 rt_mutex 的区别？HFT 为什么要关心？",
      "mutex：无优先级继承。高优先级线程等低优先级线程持有的 mutex 时，低优先级线程不会被提升 → 优先级反转 → 高优先级线程延迟增大。rt_mutex：有优先级继承。高优先级等锁时，持有者的优先级被临时提升到等待者的级别。HFT 必须用 rt_mutex（或 `PTHREAD_PRIO_INHERIT`）防止优先级反转。"),
     ("HFT 中 mutex 的使用最佳实践？",
      "① 热路径避免 mutex——用无锁设计。② 必须用 mutex 时：短临界区 + `try_lock` + 超时。③ `PTHREAD_PRIO_INHERIT` 属性防止优先级反转。④ `PTHREAD_MUTEX_ADAPTIVE_NP`：先 spin 再 sleep（glibc 扩展）。⑤ `perf lock` 分析持有时间。⑥ 避免 `std::mutex` 在 RT 线程中使用——用 `std::atomic` 或无锁队列。")]
)

NOTES[f"{C10}/section-10.6-完成变量.md"] = (
    ["混淆 completion 和 semaphore——completion 是一次性通知，semaphore 可重复",
     "在 completion 的 wait_for_completion() 中以为会自旋——它会睡眠（进程上下文）",
     "多次 complete() 一个 completion——complete() 通常只调一次，complete_all() 标记永久完成"],
    [("completion 的典型使用场景？",
      "驱动初始化等硬件就绪：`init_completion(&done)` → 启动硬件 → `wait_for_completion_timeout(&done, timeout)` → 中断处理函数中 `complete(&done)`。线程池任务完成通知：主线程 `wait_for_completion()` 等所有 worker `complete()`。模块卸载等引用归零。vs semaphore：completion 语义更清晰（一次性事件），semaphore 适合计数资源。"),
     ("complete() 和 complete_all() 的区别？",
      "complete()：唤醒一个等待者，completion 的 done+1。如果有多个等待者，需要多次 complete()。complete_all()：唤醒所有等待者，并将 completion 标记为永久完成（后续 wait_for_completion() 立即返回）。complete_all() 后不能重用该 completion（除非 reinit）。典型：驱动 probe 成功后 complete_all()，所有等待者放行。"),
     ("HFT 中 completion 的用户态对应物？",
      "① `std::promise<T>` + `std::future<T>`：一次性设置值 + 等待。② `std::condition_variable`：更灵活，可重复使用。③ `std::latch`（C++20）：一次性，多线程等同一事件。④ `std::barrier`（C++20）：多线程同步点。HFT 热路径不用这些（有 syscall 开销），用无锁标志位（`std::atomic<bool>` + spin）。")]
)

NOTES[f"{C10}/section-10.7-大内核锁.md"] = (
    ["以为 BKL 还在现代内核——2.6.37 完全移除，不存在了",
     "把 BKL 当通用锁——BKL 是特殊锁（可睡眠、递归、全局唯一），不推荐用于新代码",
     "以为 BKL 和 mutex 一样——BKL 可递归加锁、自动释放于 schedule()，mutex 不能"],
    [("BKL（Big Kernel Lock）是什么？为什么被移除？",
      "BKL 是 Linux 早期从 SMP 过渡时用的全局锁。特点：① 全局唯一（一把锁保护所有）。② 可睡眠（schedule 时自动释放，返回后重新获取）。③ 可递归（同一进程可多次 lock）。移除原因：① 全局锁是性能瓶颈（多核扩展性差）。② 可睡眠+自动释放导致语义复杂。③ 阻碍 PREEMPT_RT。2.6.37 完全移除，所有 BKL 用户改为 mutex/spinlock。"),
     ("BKL 移除后，原来用 BKL 的代码改用了什么？",
      "每个子系统逐个迁移：① ioctl → per-file mutex。② 文件系统 → per-superblock lock。③ 驱动 → per-device mutex。迁移过程持续多个版本（2.6.26-2.6.37），通过 `lock_kernel()`/`unlock_kernel()` 标记 BKL 用户，逐个替换。迁移后 SMP 扩展性显著提升。"),
     ("BKL 的历史教训对 HFT 设计有什么启示？",
      "① 避免全局锁——用 per-thread/per-CPU 数据消除共享。② 可睡眠锁不是万能的——BKL 可睡眠但导致语义混乱。③ 锁的可扩展性比锁的正确性更难——BKL 是正确的但不可扩展。④ 逐步替换优于大重写——BKL 花了 5 年逐个迁移。HFT 设计：热路径无锁，冷路径细粒度锁。")]
)

NOTES[f"{C10}/section-10.8-顺序锁.md"] = (
    ["以为 seqlock 是通用读写锁——只适合写少读多 + 读端可容忍重试的场景",
     "在读端忽略 sequence 检查——seqlock 读端必须检查前后 sequence 一致，否则可能读到半写状态",
     "在写端用多个步骤——写端持锁期间应尽快完成，持锁时间 = 写者互斥时间"],
    [("seqlock 的工作原理？读写端各做什么？",
      "写端：`write_seqlock()` → sequence++（奇数）→ 写数据 → sequence++（偶数）→ `write_sequnlock()`。写端之间互斥（spinlock）。读端：`seq1 = read_seqbegin()` → 读数据 → `seq2 = read_seqretry(seq1)` → 如果 seq1 是奇数或 seq1 != seq2 → 重读。读端无锁（不阻塞写者），但可能需要重试。"),
     ("seqlock 适合什么场景？不适合什么？",
      "适合：① 写极少读极多。② 读端可以容忍偶尔重试。③ 数据简单（几个字段，重读代价低）。典型：`jiffies`（时间戳）、`getnstimeofday()`、统计计数器。不适合：① 复杂数据结构（链表/树），重读代价高。② 写频繁（写者互斥 + 读者频繁重试）。③ 读端需要阻塞写者。这些场景用 RCU 或 rwlock。"),
     ("HFT 中 seqlock 的用户态实现？",
      "```c\n// 无锁读取时间戳\nstruct { std::atomic<uint32_t> seq; uint64_t value; } ts;\n// 写端\nuint32_t s = ts.seq.load(std::memory_order_relaxed);\nts.seq.store(s + 1, std::memory_order_release);  // 奇数\nts.value = rdtsc();\nts.seq.store(s + 2, std::memory_order_release);  // 偶数\n// 读端\nuint32_t s1, s2; uint64_t v;\ndo {\n    s1 = ts.seq.load(std::memory_order_acquire);\n    v = ts.value;\n    s2 = ts.seq.load(std::memory_order_acquire);\n} while (s1 != s2 || s1 & 1);  // 重试\n```")]
)

NOTES[f"{C10}/section-10.9-禁止抢占.md"] = (
    ["混淆 preempt_disable() 和 local_irq_disable()——前者只禁抢占，后者还禁中断",
     "以为 preempt_disable() 后不能被中断——可以被中断，但不能被调度",
     "在 preempt_disable() 区域做耗时操作——会延迟调度器，增加系统延迟"],
    [("preempt_disable() 的精确效果？",
      "① 递增 preempt_count 的 preempt 位。② 当前 CPU 上的内核代码不会被抢占（schedule() 检查 preempt_count == 0 才调度）。③ 中断仍可触发（hard IRQ）。④ softirq 仍可执行。⑤ 其他 CPU 不受影响。用于保护 per-CPU 数据（防止被另一进程在同 CPU 上访问）。对应 preempt_enable() 递减并检查 need_resched。"),
     ("preempt_enable() 时如果 need_resched 被设置会怎样？",
      "`preempt_enable()` 递减 preempt_count，如果 preempt_count 归零且 `need_resched` 被设置 → `preempt_schedule()` → `schedule()` 切换到更高优先级任务。这就是内核抢占点。`preempt_enable_no_resched()` 不检查 need_resched（延迟到下一个抢占点），用于明确不需要立即调度的场景。"),
     ("HFT 如何利用抢占控制降低延迟？",
      "① `SCHED_FIFO`：RT 线程不可被 CFS 抢占（只有更高 RT 或中断能抢占）。② `isolcpus`：隔离核上无其他任务，调度器几乎不触发。③ `nohz_full`：停止定时器中断，减少 `scheduler_tick()`。④ `preempt=full`：让非 RT 任务的内核路径也可被抢占（减少长尾延迟）。⑤ 内核模块中 `preempt_disable()` 临界区 <1us。")]
)

NOTES[f"{C10}/section-10.10-排序和屏障.md"] = (
    ["混淆 smp_mb() / smp_rmb() / smp_wmb()——全屏障/读屏障/写屏障，保证不同方向的重排",
     "以为 x86 不需要 memory barrier——x86 有 TSO 内存模型，大部分 barrier 是空操作，但 store-load 重排仍需要 mfence",
     "在 UP 上用 smp_mb()——UP 上 smp_mb() 是空操作（无 SMP 重排），应改用 barrier() 或不需要"],
    [("smp_mb() / smp_rmb() / smp_wmb() 分别保证什么？",
      "smp_mb()：全屏障，之前的读写 + 之后的读写都不可跨屏障重排。smp_rmb()：读屏障，之前的读不可重排到之后的读之后。smp_wmb()：写屏障，之前的写不可重排到之后的写之后。x86 TSO 模型下：smp_rmb() = 空操作（loads 不重排），smp_wmb() = 空操作（stores 不重排），smp_mb() = `mfence`（禁止 store-load 重排）。ARM64：三者都是真实指令（`dmb ish`/`dmb ishld`/`dmb ishst`）。"),
     ("smp_store_release() / smp_load_acquire() 相比 smp_mb() 有什么优势？",
      "smp_store_release(ptr, val)：等价于 smp_wmb() + WRITE_ONCE(*ptr, val)，只保证之前的读写不重排到这个 store 之后。smp_load_acquire(ptr)：等价于 READ_ONCE(*ptr) + smp_rmb()，只保证之后的读写不重排到这个 load 之前。优势：① 更精细——只关联一个操作，不影响其他操作。② 在 x86 上 release = 普通 store（无开销），acquire = 普通 load（无开销）。③ 代码更清晰。"),
     ("HFT 中 memory barrier 误用会导致什么问题？",
      "① 缺少 barrier：消息传递 pattern 失败——`data = x; ready = true;` 如果 CPU 重排为 `ready = true; data = x;`，消费者看到 ready=true 但 data 还是旧值。② 过多 barrier：性能下降——每个 smp_mb() 在 x86 上是 `mfence`（~30 cycles），ARM64 上 `dmb`（~50 cycles）。HFT 无锁队列应精确用 release/acquire 替代 seq_cst。用 `std::atomic` + 正确的 memory_order 避免手动 barrier。")]
)

NOTES[f"{C10}/section-10.11-选型速查Ch-9--Ch-10.md"] = (
    ["在所有场景都用 spinlock——短临界区用 spinlock，长临界区用 mutex",
     "忽略 RCU——读极多写极少时 RCU 是最优解（读端零开销）",
     "忘记 lockdep——开发阶段开 lockdep 检测死锁/锁顺序问题"],
    [("给定场景，如何快速选择同步原语？",
      "中断上下文 → spin_lock_irqsave()。softirq → spin_lock_bh()。进程上下文 + 短临界区（<1us） → spin_lock()。进程上下文 + 长临界区 → mutex。读极多写极少 + 简单数据 → seqlock。读极多写极少 + 复杂数据 → RCU。一次性等待 → completion。引用计数 → refcount_t。不确定 → spin_lock_irqsave()（最安全）。"),
     ("同步原语的性能排序？",
      "最快 → 最慢：① atomic 操作（~20ns）。② RCU 读端（~0ns，只禁抢占）。③ seqlock 读端（~10ns）。④ spinlock 无争用（~20ns）。⑤ rwlock 读端无争用（~20ns）。⑥ mutex 无争用（~20ns）。⑦ spinlock 有争用（~100ns-spin）。⑧ mutex 有争用（~1-5us，schedule）。⑨ RCU 写端（~ms，等 grace period）。选择：热路径用 ①-⑤，冷路径可用 ⑥-⑨。"),
     ("HFT 同步原语选型决策树？",
      "```\n热路径？\n├─ 是 → 数据可 per-thread？\n│       ├─ 是 → 无锁（per-thread 变量）\n│       └─ 否 → SPSC 队列？\n│               ├─ 是 → atomic<head,tail> + release/acquire\n│               └─ 否 → 分片锁 / 无锁哈希表\n└─ 否 → 临界区 <1us？\n        ├─ 是 → spinlock / atomic\n        └─ 否 → mutex（+ rt_mutex 优先级继承）\n```")]
)

# === Ch11 Timers (7 notes) ===
C11 = "chapter-11-timers/notes"
NOTES[f"{C11}/section-11.1-内核时间概念与节拍率.md"] = (
    ["以为 HZ 越高越好——高 HZ 增加定时器中断频率，浪费 CPU + 影响缓存",
     "混淆 jiffies 和 wall time（墙上时间）——jiffies 是内核启动后的 tick 数，wall time 是真实时间",
     "以为 NO_HZ 就是禁用所有定时器中断——NO_HZ 只在 CPU idle 或 single-task 时禁用周期性 tick"],
    [("HZ 的值对系统有什么影响？HFT 应该用多少？",
      "HZ=100：每 10ms 一次 tick，调度精度低，中断开销小（服务器默认）。HZ=1000：每 1ms 一次 tick，调度精度高，中断开销大（桌面默认）。HZ=250：折中。HFT：① 交易核用 `nohz_full`（停止 tick）。② 非 RT 核 HZ=100 即可（减少中断）。③ 需要 ns 级精度用 hrtimer（不依赖 HZ）。④ `CONFIG_HZ=100` + `nohz_full=N` 最优。"),
     ("jiffies 和 wall time 的区别？怎么转换？",
      "jiffies：内核启动后的 tick 计数（unsigned long），每次 tick +1。wall time：真实世界时间（struct timespec / timeval）。转换：`jiffies_to_msecs(j)` / `jiffies_to_usecs(j)`。`jiffies_64` 是 64 位版本。wall time 通过 `do_gettimeofday()` / `ktime_get_real_ts()` 获取。HFT 不用 jiffies（精度太低），用 `ktime_get()` 或 TSC。"),
     ("NO_HZ / nohz_full 对 HFT 的意义？",
      "NO_HZ（tickless idle）：CPU idle 时停止周期性 tick，省电。nohz_full=N：N 号 CPU 上只有一个任务时停止 tick，减少中断。对 HFT：① 消除每 1/10ms 的 `scheduler_tick()` 中断。② 减少 context switch。③ 降低 cache 污染。配置：`nohz_full=2-3 isolcpus=2-3 rcu_nocbs=2-3`。注意：该核上不能有多个竞争 CPU 的任务。")]
)

NOTES[f"{C11}/section-11.2-jiffies-变量.md"] = (
    ["混淆 jiffies 和 jiffies_64——jiffies 是 32 位（可能回绕），jiffies_64 是 64 位",
     "用 jiffies 做精确计时——jiffies 精度 = 1/HZ（1ms 或 10ms），不适合纳秒级计时",
     "忽略 jiffies 回绕——32 位 jiffies 在 HZ=1000 时 ~49 天回绕，用 time_after() 比较"],
    [("jiffies 回绕是什么问题？怎么安全比较时间？",
      "jiffies 是 unsigned long（32 位），HZ=1000 时 ~49.7 天回绕（2^32 / 1000 / 86400 ≈ 49.7）。直接比较 `jiffies > deadline` 在回绕时出错。安全比较：`time_after(jiffies, deadline)` 和 `time_before(jiffies, deadline)`——内部用有符号差值处理回绕。`time_in_range(jiffies, start, end)` 检查是否在范围内。"),
     ("为什么 jiffies 不适合 HFT 计时？",
      "① 精度低：HZ=1000 时只有 1ms 精度，HFT 需要纳秒级。② 不是单调的：wall time 可被 NTP 调整（跳变）。③ 32 位回绕风险。HFT 应使用：① `ktime_get()` / `ktime_get_ns()`：纳秒级，单调。② `rdtsc()` / `__rdtsc()`：CPU TSC，纳秒级，最快（~20ns）。③ `clock_gettime(CLOCK_MONOTONIC)`：走 vDSO，~20ns。"),
     ("HFT 如何用 TSC 做精确计时？",
      "```c\n#include <x86intrin.h>\nuint64_t t1 = __rdtsc();  // 读 TSC\n// ... 待测代码 ...\nuint64_t t2 = __rdtsc();\ndouble ns = (double)(t2 - t1) / tsc_frequency_ghz;\n// 获取 TSC 频率: cat /proc/cpuinfo | grep MHz\n// 注意: 1) TSC 在现代 CPU 上是不变的(invariant)\n//       2) 多核间 TSC 同步(但老 CPU 可能不同步)\n//       3) 用 RDTSCP 替代 RDTSC 保证顺序\n```")]
)

NOTES[f"{C11}/section-11.3-硬件时钟和定时器.md"] = (
    ["混淆「时钟源」（clocksource）和「时钟事件设备」（clock_event_device）——前者只读时间，后者可触发中断",
     "以为 HPET 是最佳时钟源——TSC 比 HPET 快 100 倍（~20ns vs ~2us），HPET 是后备",
     "忽略时钟源的稳定性——TSC 在老 CPU 上可能不稳定（频率变化/多核不同步）"],
    [("clocksource 和 clock_event_device 的区别？",
      "clocksource：只读时钟（单调递增），用于读取当前时间。如 TSC、HPET、ACPI PM Timer。选择最快且稳定的。clock_event_device：可编程定时器，用于设置下一次中断。如 Local APIC Timer、HPET。一个 CPU 上有一个 clocksource（全局共享）和一个 clock_event_device（per-CPU）。"),
     ("TSC vs HPET vs ACPI PM Timer 的性能对比？",
      "TSC（Time Stamp Counter）：~20ns 读取，不变 TSC（invariant TSC）在现代 CPU 上稳定。首选。HPET：~2us 读取，高精度但慢。后备（TSC 不稳定时使用）。ACPI PM Timer：~2us，最后备选。`cat /sys/devices/system/clocksource/clocksource0/current_clocksource` 查看当前使用。HFT 确保 `tsc` 而非 `hpet`。内核启动参数 `clocksource=tsc`。"),
     ("HFT 如何确保 TSC 可靠？",
      "① `cat /proc/cpuinfo | grep constant_tsc`：确认 invariant TSC。② `dmesg | grep -i tsc`：检查内核是否标记 TSC 为 unstable。③ `tsc_reliable` 启动参数：强制标记 TSC 可靠。④ 多核 TSC 同步：现代 CPU 在启动时同步 TSC（`sync_tsc()`），但不同 socket 可能有偏差。⑤ HFT 绑核在同一 socket 上避免跨 socket TSC 偏差。⑥ `RDTSCP` 指令比 `RDTSC` 多一个序列化保证。")]
)

NOTES[f"{C11}/section-11.4-定时器中断处理程序.md"] = (
    ["把 ULK 的定时器中断处理当现代版——6.x 用 hrtimer + NO_HZ + tickless 机制完全不同",
     "混淆 scheduler_tick() 和 timer tick——scheduler_tick() 是 tick 中断的一部分，更新调度统计",
     "以为定时器中断频率固定——NO_HZ 下可以动态调整或完全停止"],
    [("现代内核的定时器中断处理和 ULK 时代有什么区别？",
      "ULK 时代：固定 HZ 频率的 tick → `do_timer_interrupt()` → 更新 jiffies + scheduler_tick() + 检查 timer list。现代：① hrtimer 框架取代 timer list（精度从 ms 提升到 ns）。② NO_HZ：idle 时停止 tick。③ nohz_full：single-task 时停止 tick。④ tickless：动态计算下一个需要的 tick 时间。⑤ `tick_nohz_idle_enter()` / `tick_nohz_idle_exit()`。"),
     ("scheduler_tick() 做什么？对 HFT 有什么影响？",
      "① 更新当前进程的 vruntime / 时间统计。② 检查时间片是否耗尽 → 设 need_resched。③ 更新 CPU 负载统计。④ 触发 RT 负载均衡检查。影响：每次 tick 中断交易线程 ~1-5us。HFT 用 `nohz_full` 消除交易核的 tick → scheduler_tick() 不执行 → 交易线程不被中断。"),
     ("HFT 如何配置 tickless 减少定时器中断？",
      "```bash\n# 内核启动参数\nisolcpus=2-3 nohz_full=2-3 rcu_nocbs=2-3\n# 确认\ncat /sys/devices/system/cpu/nohz_full\n# 应输出 2-3\ncat /proc/interrupts | grep LOC\n# 2-3 号 CPU 的 LOC（local timer）计数应几乎不变\n# 注意: nohz_full CPU 上只能跑一个任务(或 RT 线程组)\n```")]
)

NOTES[f"{C11}/section-11.5-实际时间-墙上时间.md"] = (
    ["混淆 CLOCK_REALTIME 和 CLOCK_MONOTONIC——前者可被 NTP 调整（会跳变），后者不会",
     "用 CLOCK_REALTIME 做计时——NTP 调整可能导致时间倒流，计时段为负",
     "在内核中用 do_gettimeofday()——已废弃，应用 ktime_get_real_ts() / ktime_get()"],
    [("CLOCK_REALTIME / CLOCK_MONOTONIC / CLOCK_MONOTONIC_RAW 的区别？",
      "REALTIME：墙上时间（1970-01-01 起的秒数），可被 NTP/settimeofday 调整，可能跳变。MONOTONIC：单调递增，不受 NTP 调整影响（但受 NTP 频率调整影响，可能快慢漂移）。MONOTONIC_RAW：纯硬件时钟，完全不受 NTP 影响。HFT 用 MONOTONIC（单调 + 不跳变）。`clock_gettime(CLOCK_MONOTONIC, &ts)` 走 vDSO（~20ns）。"),
     ("为什么 HFT 不用 CLOCK_REALTIME？",
      "① NTP 调整可能导致时间跳变（向前或向后）→ 计时段为负或异常大。② `settimeofday()` 可被 root 手动设置 → 不可预测。③ 跨机器时间同步需要 NTP/PTP，但同步应通过 PTP 硬件时间戳在应用层处理，不依赖系统时钟。HFT 用 MONOTONIC 做本地计时，用 PTP 做跨机器同步。"),
     ("HFT 如何获取纳秒级单调时间？",
      "```c\n#include <time.h>\nstruct timespec ts;\nclock_gettime(CLOCK_MONOTONIC, &ts);\n// 走 vDSO, ~20ns, 不进内核\n// 或直接 RDTSC:\nuint64_t tsc = __rdtsc();\nuint64_t ns = tsc * 1000 / tsc_khz;\n// 需要校准 TSC 频率: tsc_khz = 基准频率 * 1000\n// HFT 最佳: RDTSC + 预校准频率, 延迟 < 50ns\n```")]
)

NOTES[f"{C11}/section-11.6-动态定时器.md"] = (
    ["混淆 timer_list（低精度）和 hrtimer（高精度）——现代内核优先 hrtimer",
     "以为定时器回调在中断上下文执行——timer_list 在 softirq 上下文，hrtimer 在 hard IRQ 或 softirq",
     "在定时器回调中睡眠——timer_list 回调在 softirq 上下文不能睡眠，hrtimer 也不能"],
    [("timer_list 和 hrtimer 的区别？现代内核推荐用哪个？",
      "timer_list：低精度（ms 级，基于 jiffies/tick），回调在 TIMER_SOFTIRQ 上下文。hrtimer：高精度（ns 级，基于 hrtimer 框架 + clock_event_device），回调在 hard IRQ 或 HRTIMER_SOFTIRQ 上下文。现代内核推荐 hrtimer——精度更高，且 NO_HZ 模式下 timer_list 也被 hrtimer 模拟。新代码应始终用 hrtimer。"),
     ("hrtimer 回调函数为什么不能睡眠？",
      "hrtimer 回调在 hard IRQ 或 softirq 上下文执行，无 task_struct、不可调度。睡眠需要 schedule()，但 preempt_count != 0 时 schedule() 会 panic。如果需要在定时器回调中做可睡眠操作：① hrtimer 回调返回 HRTIMER_RESTART → 在 softirq 中重新调度 → workqueue 处理。② 或用 delayed_work（基于 timer_list + workqueue）。"),
     ("HFT 中定时器的使用场景和替代方案？",
      "场景：超时检测、心跳发送、定期采样。替代方案：① 用户态用 `timerfd_create()` + `epoll`（可合并到事件循环）。② 轮询模式（DPDK）：不用定时器，主循环中检查 TSC。③ `SCHED_FIFO` 线程 + `clock_nanosleep()`：精确睡眠（但仍有调度延迟）。④ 自旋等待 + RDTSC：最精确但浪费 CPU。HFT 热路径用 ③/④，非热路径用 ①。")]
)

NOTES[f"{C11}/section-11.7-延迟执行.md"] = (
    ["混淆 udelay() 和 msleep()——前者忙等（spin，精确但浪费 CPU），后者睡眠（释放 CPU 但有调度延迟）",
     "在持锁时用 msleep()——spinlock 持有时不能睡眠，mutex 可以",
     "在 HFT 热路径用 sleep/wait——热路径应预分配 + 无等待，delay API 都是后备"],
    [("udelay() / mdelay() / msleep() / schedule_timeout() 的区别？",
      "udelay(us)：忙等（spin），基于 BogoMIPS 校准的循环，精度 ns 级，不释放 CPU。mdelay(ms)：udelay 的毫秒版。msleep(ms)：睡眠（schedule_timeout + TASK_UNINTERRUPTIBLE），释放 CPU，精度 ms 级 + 调度延迟。schedule_timeout(timeout)：可设置 TASK_INTERRUPTIBLE/UNINTERRUPTIBLE，最灵活。选择：<10us → udelay；>10us 且可睡眠 → msleep/schedule_timeout。"),
     ("为什么 spinlock 持有时只能 udelay 不能 msleep？",
      "Spinlock 持有时 preempt_count > 0（或中断禁用）。msleep() 内部调 schedule()，schedule() 检查 preempt_count == 0 才允许调度。违反 → BUG: scheduling while atomic → panic。mutex 持有时 preempt_count == 0（mutex 不禁抢占），可以 msleep()。但如果 mutex 持有时睡眠，其他等待者被阻塞。"),
     ("HFT 用户态如何做精确延迟？",
      "```c\n// 方法 1: 自旋 + RDTSC（最精确，浪费 CPU）\nuint64_t target = rdtsc() + ns * tsc_ghz;\nwhile (rdtsc() < target) _mm_pause();  // PAUSE 指令降功耗\n// 方法 2: nanosleep（释放 CPU，有调度延迟）\nstruct timespec ts = { .tv_sec = 0, .tv_nsec = ns };\nnanosleep(&ts, NULL);  // 最小 ~50us（调度开销）\n// 方法 3: futex  spin（自适应）\n// HFT 热路径: 方法 1（自旋）, <1us 精确\n// HFT 非热路径: 方法 2, 节省 CPU\n```")]
)

# ============================================================
# Main
# ============================================================
ok = 0
miss = 0
skip = 0

for rel_path, (traps, quiz) in NOTES.items():
    fpath = os.path.join(BASE, rel_path)
    if not os.path.exists(fpath):
        print(f"MISSING: {rel_path}")
        miss += 1
        continue

    with open(fpath, "r", encoding="utf-8") as f:
        content = f.read()

    if "### 常见陷阱" in content:
        print(f"SKIP: {rel_path}")
        skip += 1
        continue

    block = make_block(traps, quiz)
    new_content = insert_before_last_sep(content, block)

    with open(fpath, "w", encoding="utf-8") as f:
        f.write(new_content)

    print(f"OK: {rel_path}")
    ok += 1

print(f"\n=== Done: {ok} OK, {miss} MISSING, {skip} SKIP ===")
