# 7.3 BPF 工具（一）：oomkill

> 底本：《BPF之巅》第 7 章 内存，7.3.1 节（印刷 p270–271）。7.3 开篇：图 7-4 全景 + 表 7-3（11 个工具：oomkill/memleak/mmapsnoop/brkstack/shmsnoop/faults/ffaults/vmscan/drsnoop/swapin/hfaults）。

## 7.3 工具总表（表 7-3）

| 工具 | 目标 | 功能 | 来源 |
|------|------|------|------|
| oomkill | OOM | OOM Killer 事件详情 | BCC/BT |
| memleak | 分配 | 疑似泄漏的代码路径 | BCC |
| mmapsnoop | 系统调用 | 全系统 mmap(2) 跟踪 | 本书(BT) |
| brkstack | 系统调用 | brk 调用的用户态栈 | 本书(BT) |
| shmsnoop | 系统调用 | SysV 共享内存调用 | BCC |
| faults | 缺页 | 按用户栈统计缺页 | 本书 |
| ffaults | 缺页 | 按文件名统计缺页 | 本书(BT) |
| hfaults | 缺页 | 按进程统计巨页缺页 | 本书(BT) |
| vmscan | VM | 扫描器收缩/回收时间 | 本书(BT) |
| drsnoop | VM | 直接回收延迟 | BCC |
| swapin | VM | 按进程统计页换入 | 本书(BT) |

（内核内存的 kmem/kpages/slabratetop/numamove 见第 14 章。）

## oomkill — OOM 事件跟踪

BCC + bpftrace 工具：跟踪 OOM Kill 事件并打印**触发者、受害者、页数、平均负载**（负载提供 OOM 时系统整体状态上下文：变忙还是平稳）。

```
# oomkill
Tracing OOM kills... Ctrl-c to stop.
08:51:34 Triggered by PID 18601 ("perl"), OOM kill of PID 1165 ("java"), 18006224 pages,
         loadavg: 10.66 7.17 5.06 2/7551 8643
```

解读：perl 申请内存触发 OOM → **java 被杀**（占内存最多者），它已占 1800 万页（≈4KB/页 ≈ 68GB）。负载 1/5/15 = 10.66/7.17/5.06 上升 → 系统正在变忙。

- 实现：kprobe `oom_kill_process()`（bpftrace 版将 arg1 转型为 `struct oom_control` 取受害者详情；负载用 `cat("/proc/loadavg")`）
- 开销：OOM 极低频 → 可忽略；**适合 7×24 常驻**，捕获偶发 OOM
- 可扩展方向：内核 oom 跟踪点可展示任务选择的更多细节

bpftrace 版核心：

```bash
#!/usr/local/bin/bpftrace
#include <linux/oom.h>
BEGIN { printf("Tracing oom_kill_process(... Hit Ctrl-C to end.\n"); }
kprobe:oom_kill_process
{
    $oc = (struct oom_control *)arg1;
    time("%H:%M:%S");
    printf("Triggered by PID %d (\"%s\"), ", pid, comm);
    printf("OOM kill of PID %d (\"%s\"), %d pages, loadavg: ",
           $oc->chosen->pid, $oc->chosen->comm, $oc->totalpages);
    cat("/proc/loadavg");
}
```

## HFT 关联

- 交易机 oomkill **必须常驻**：无 swap 环境下 OOM 是唯一内存故障模式；捕获"谁触发、谁被杀"比事后 dmesg 更完整（含负载上下文）
- 被杀的往往不是肇事者（perl 触发、java 被杀）— 事后追责需要 oomkill 的 Triggered by 信息
- 关键进程设 `oom_score_adj = -1000` 免死；监控 agent 用 oomkill 事件驱动告警

## 常见陷阱

1. **以为被杀进程就是肇事者** — OOM Killer 按占内存最多选牺牲品，触发者（Triggered by）可能是另一个小进程
2. **只靠 dmesg 事后查** — dmesg 有信息但缺实时性；oomkill 常驻可第一时间告警并保留上下文
3. **忽略 loadavg 上下文** — OOM 时负载上升说明内存压力正在引发调度问题，是根因分析的线索

<details>
<summary>📝 自测题（点击展开）</summary>

1. **oomkill 的输出里 "Triggered by" 和 "OOM kill of" 分别是什么进程？**

   <details>
   <summary>参考答案</summary>

   Triggered by = 触发 OOM 的进程（它申请内存时系统耗尽，即 current 进程，kprobe 上下文的 pid/comm）；OOM kill of = 被杀的牺牲品（oom_control->chosen，按占内存最多等规则选出）。两者经常不是同一个进程。
   </details>

2. **为什么 oomkill 适合常驻运行而 memleak 不适合？**

   <details>
   <summary>参考答案</summary>

   oomkill 跟踪 oom_kill_process()，OOM 事件极低频（数月一次），kprobe 附加几乎无代价。memleak 跟踪 malloc/free，每秒数百万次，开销可达 10% 甚至 10 倍减速，只能做调试工具。
   </details>

</details>
