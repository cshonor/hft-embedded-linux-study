# 6.3 BPF 工具（一）：进程生命周期 — execsnoop / exitsnoop

> 底本：《BPF之巅》第 6 章 CPU，6.3.1–6.3.2 节（印刷 p211–215）。6.3 开篇有全景图（图 6-4）与工具总表（表 6-3，17 个工具），本笔记覆盖前两个。

## 6.3 工具总表速览（表 6-3 节选）

| 工具 | 类别 | 描述 | 来源 |
|------|------|------|------|
| execsnoop | 调度 | 跟踪新进程执行 | BCC/BT |
| exitsnoop | 调度 | 跟踪进程退出与退出原因 | BCC |
| runqlat | 调度 | 运行队列延迟直方图 | BCC/BT |
| runqlen | 调度 | 运行队列长度采样 | BCC/BT |
| runqslower | 调度 | 延迟超过阈值的线程 | BCC |
| cpudist | 调度 | 线程在 CPU 上执行时长分布 | BCC |
| cpufreq | 调度 | CPU 频率采样 | BT |
| profile | 定时采样 | CPU 调用栈剖析 | BCC/BT |
| offcputime | 调度 | 脱离 CPU 时间 + 阻塞栈 | BCC/BT |
| syscount | 系统调用 | 按类型/进程统计 | BCC/BT |
| softirqs / hardirqs | 中断 | 软/硬中断耗时 | BCC |
| smpcalls | 内核 | SMP 跨 CPU 调用 | BT |
| llcstat | PMC | LLC 命中率 | BCC |

## 6.3.1 execsnoop — 跟踪新进程

**用途**：抓取**短命进程**（top/监控抓取间隔内就退出的进程），分析启动脚本、cron、软件执行流程。

```
PCOMM    PID    PPID  RET ARGS
sshd     33096  2366  0   /usr/sbin/sshd -D -R
bash     33118  33096 0   /bin/bash
ls       33121  33119 0   /bin/ls /etc/bash_completion.d
sadc     33144  33143 0   /usr/lib/sysstat/sadc -F -L -sDISK 11 /var/lo..
```

原理与边界：

- 直接跟踪 **`execve(2)`** 系统调用（最常用 exec 变体），打印参数与返回值
- 抓得到 `fork/clone → exec` 的新进程和自己调 exec 的进程
- **抓不到纯 fork/clone 不 exec 的进程池**（不常见；应用一般应该用线程池）
- 进程创建频率一般 <1000 次/秒 → **开销可忽略**
- 输出含 PPID → 能还原整棵进程树（书中 SSH 登录触发 sshd→bash→groups→mesg 链的例子）

BCC 选项：

| 选项 | 作用 |
|------|------|
| `-x` | 包含 exec 失败的情况 |
| `-n pattern` | 只输出命令名匹配的结果 |
| `-l pattern` | 只输出参数匹配的结果 |
| `--maxargs N` | 参数个数上限（默认 20） |

bpftrace 核心实现：

```bash
#!/usr/local/bin/bpftrace
BEGIN { printf("%-10s %-5s %s\n", "TIME(ms)", "PID", "ARGS"); }
tracepoint:syscalls:sys_enter_execve
{
    printf("%-10u %-5d", elapsed/1000000, pid);
    join(args->argv);
}
```

> join() 就是作者为写 execsnoop 而加入 bpftrace 的内置函数。BCC 版同时跟踪入口+返回点，可输出 RET 列（bpftrace 版可自行扩展）。

## 6.3.2 exitsnoop — 跟踪进程退出

**用途**：打印进程退出时的**总运行时长（AGE）和退出原因**，从另一个角度分析短命进程（进程为什么死、死了多少次、被谁 kill）。

```
PID    TID    PPID   PCOMM     AGE(s)  EXIT CODE
8994   8994   8993   cmake     0.01    0
8967   8967   26663  sleep     7.31    signal 9 (KILL)
4183   8301   5111   DOMWorker 221.25  0
```

- 用 `sched:sched_process_exit` 跟踪点 + `bpf_get_current_task()` 读 task 结构体起始时间（**非稳定接口**，内核升级可能变）
- AGE 含 CPU 时间和非运行时间（从创建到终止）
- 跟踪点本身低频 → 开销可忽略

BCC 选项：`-p PID`（仅该进程）、`-t`（时间戳）、`-x`（仅失败退出，退出码 ≠ 0）。

> 目前**没有 bpftrace 版 exitsnoop** — 原书把它留作练习（见 section-11 可选练习）。

## HFT 关联

- 交易机上的**监控脚本风暴**是经典问题：每秒 spawn 一堆 shell/awk/grep，每次都有 fork+exec 开销 + 缓存污染。execsnoop 一跑全现形
- 策略进程异常退出/被 OOM killer 或 systemd 重启 → exitsnoop 的 `signal 9` 行直接给出证据
- execsnoop 抓不到的 fork 进程池：改用 sched 跟踪点（原书练习 9，procsnoop）

## 常见陷阱

1. **以为 execsnoop 能看到所有新进程** — 只覆盖走 execve(2) 的；纯 fork/clone 工作进程池不可见
2. **AGE ≠ CPU 时间** — exitsnoop 的 AGE 是墙上时间总寿命，含睡眠阻塞时间
3. **忽略 exec RET 非 0** — 大量 exec 失败（脚本里命令拼错/路径不存在）也在烧 CPU，加 `-x` 单独看
4. **一次性 execsnoop 输出洪水后不做统计** — 配合 `-n`/`-l` 过滤，或落盘后 sort | uniq -c 找高频命令

<details>
<summary>📝 自测题（点击展开）</summary>

1. **execsnoop 为什么抓不到某些进程创建？举一个会漏掉的场景。**

   <details>
   <summary>参考答案</summary>

   它跟踪 execve(2)，只覆盖"执行了新程序映像"的进程。漏掉：仅用 fork(2)/clone(2) 复制自身、不 exec 的工作进程池（如某些数据库/服务器 prefork 模型）。这类需求要跟踪 sched:sched_process_fork 跟踪点。
   </details>

2. **exitsnoop 是怎么算出进程 AGE 的？有什么风险？**

   <details>
   <summary>参考答案</summary>

   sched:sched_process_exit 触发时，用 bpf_get_current_task() 从 task_struct 读进程创建时间戳求差。风险：直接读 task 结构体属于非稳定接口，内核版本变化（字段重命名/结构调整）会导致工具失效，需同步维护。
   </details>

</details>
