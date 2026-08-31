# 1.9 再回到 BCC：跟踪 open()

> 底本：《BPF之巅》中文版 1.9 节（PDF p53–54）

## BCC 版 opensnoop(8)

输出列与 bpftrace 版一致（PID/COMM/FD/ERR/PATH），但**命令行参数丰富得多**：

```text
# opensnoop -h
usage: opensnoop [-h] [-T] [-x] [-p PID] [-t TID] [-d DURATION]
                 [-n NAME] [-e] [-f FLAG_FILTER]

  -T, --timestamp       include timestamp on output
  -x, --failed          only show failed opens
  -p PID, --pid PID     trace this PID only
  -t TID, --tid TID     trace this TID only
  -n NAME, --name NAME  only print process names containing this name
  -e, --extended        show extended fields
  -f FLAG_FILTER        filter on flags argument (e.g. O_WRONLY)
```

实战示例——只看失败的 open：

```text
# opensnoop -x
PID    COMM             FD   ERR PATH
991    irqbalance       -1   2   /proc/irq/133/smp_affinity
991    irqbalance       -1   2   /proc/irq/141/smp_affinity
20543  systemd-resolve  -1   2   /run/systemd/netif/links/5
20543  systemd-resolve  -1   2   /run/systemd/netif/links/5
...
```

**不断重复的打开失败**——可能指向程序效率问题或可修复的配置错误（ERR=2 即 ENOENT：文件不存在）。

参数背后的设计思路值得注意：**bpftrace 版要改源码才能实现的过滤，BCC 版做成了参数**。这不是简单堆功能——过滤发生在**内核态 BPF 程序里**（`-p PID` 是把 pid 写进 map、BPF 程序里比对后丢弃），不满足条件的事件根本不进输出通道。对比"全部打印到用户态再 grep"是量级的差别：过滤前置到内核，输出通道零浪费。

## ERR 列速查（排障时直接对表）

| ERR | 常见含义 | 典型场景 |
|---|---|---|
| 2 ENOENT | 文件/路径不存在 | 配置漂移、挂载点变化、路径拼错 |
| 13 EACCES | 权限拒绝 | 运行用户与文件属主不匹配（容器里高发） |
| 21 EISDIR | 是目录 | 把目录当文件打开 |
| 24 EMFILE | 进程 fd 用尽 | fd 泄漏（配合 closesnoop/exitsnoop 找泄漏点） |
| 11 EAGAIN | 非阻塞模式下暂不可用 | 打开了 FIFO/套接字类特殊文件 |

## BCC 工具的形态：一个文件两截代码

BCC 工具是 Python 脚本，但内嵌 C 源码字符串（示意，opensnoop 简化版）：

```python
#!/usr/bin/python
from bcc import BPF
b = BPF(text='''
#include <uapi/linux/ptrace.h>
TRACEPOINT_PROBE(syscalls, sys_enter_openat) {
    bpftrace_printk("%s\\n", args->filename);   // 内核态：C，事件发生时执行
    return 0;
}
''')
b.trace_print()                                   # 用户态：Python，收事件打印
```

- `BPF(text=...)` 触发**运行时编译**（Clang/LLVM）→ 加载 → attach
- 内核态 C 与用户态 Python 的通信走 perf buffer/ring buffer 或 map——这就是"一个工具、两个执行域"的直观形态
- 改行为 = 改源码；这也是 BCC 工具天然适合"fork 一份定制"的原因（70+ 工具都是可读可改的模板）

## BCC vs bpftrace 分工（本章结论性对比）

| | bpftrace 工具 | BCC 工具 |
|---|---|---|
| 风格 | 简单、功能单一、做一件事 | 复杂、多运行模式 |
| 过滤/选项 | 要改源码（如只显示失败 open 得改脚本） | 命令行参数直接支持（`-x`） |
| 定位 | 定制工具、快速问答 | **工作起点**——需要的功能多半已自带 |
| 演进路径 | bpftrace 原型 → 成熟后改写为带参数的 BCC 工具 | BCC 还能组合多事件源：优先 tracepoint、不满足再退 kprobe |

> BCC 编程复杂度高，本书正文聚焦 bpftrace 编程；**附录 C** 提供 BCC 开发快速入门。

---

### HFT 关联

- `-p PID` 按进程过滤是交易机必备：观测窗口只盯策略进程，不把系统其他噪声（监控 agent、日志切割）混进来——且过滤在内核态完成，噪声事件不占输出带宽
- `opensnoop -x` 是发现**配置漂移**的利器：容器化交易组件挂载路径变化后，反复 ENOENT 的重试循环会直接烧 CPU 并拖慢初始化
- ERR 列是"免费的健康度信号"：定期抓一段 `-x` 输出做 diff，EMFILE 增长 = fd 泄漏前兆（交易网关的常见慢性病）
- bpftrace（原型验证）→ BCC/libbpf（产品化）的演进路径，对应 HFT 观测工具的迭代纪律：先证明指标有用，再工程化常驻
- 交叉引用：BCC 内部机制（运行时编译、libbcc 架构）详解见 [Ch4 BCC](../../chapter-04-bcc/) 与 [Learning eBPF Ch5](../../../01-learning-ebpf/chapter-05-core-btf-libbpf/)

<details>
<summary>📝 自测题（点击展开）</summary>

1. **`opensnoop -x` 输出中 ERR=2 表示什么？这种模式为何值得警惕？**

   <details><summary>参考答案</summary>

   2 = ENOENT（文件不存在）。若同一进程对同一路径反复失败打开，说明配置错误或路径漂移，重试循环本身消耗 CPU，且常意味着程序没拿到它需要的数据（配置、证书、共享文件）。

   </details>

2. **什么信号说明该把 bpftrace 脚本升级为 BCC 工具？**

   <details><summary>参考答案</summary>

   当脚本需要反复使用、需要精细命令行参数（按 PID/TID/时长/失败过滤）、要作为后台进程长跑或与其他事件源组合时——bpftrace 改源码的成本开始超过一次性收益。

   </details>

3. **BCC 的 `-p PID` 过滤和"输出后再 `grep PID"`有什么本质区别？**

   <details><summary>参考答案</summary>

   BCC 的过滤在内核态 BPF 程序内完成（pid 写入 map，程序里比对丢弃）——不满足的事件不进输出通道，不占 perf buffer、不叫醒用户态。grep 是全量事件都走完"内核→用户态"通路后才丢弃：开销差着"事件率 × 通路成本"的量级。

   </details>

4. **BCC 工具源码里为什么同时有 Python 和 C？两者各在哪个执行域、怎么通信？**

   <details><summary>参考答案</summary>

   C 部分编译成 BPF 程序在内核态事件发生时执行（采集/过滤/聚合）；Python 部分在用户态负责编译加载、参数解析、收输出。通信走 perf/ring buffer（事件流）或 map（聚合结果、控制参数如 pid 过滤值）。

   </details>

</details>
