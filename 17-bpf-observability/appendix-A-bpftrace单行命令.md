# 附录 A bpftrace单行命令 · bpftrace One-Liners

> **BPF Performance Tools** · Brendan Gregg · **精读**

## 按资源域分类的常用 One-Liners

### CPU

```bash
# 统计内核函数调用次数
bpftrace -e 'kprobe:do_sys_open { @++ }'

# CPU 采样火焰图数据
bpftrace -e 'profile:hz:99 { @[kstack] = count() }'

# 调度延迟直方图（runqueue latency）
bpftrace -e 'tracepoint:sched:sched_wakeup { @s[tid]=nsecs } tracepoint:sched:sched_switch /@s[args->next_pid]/ { @runqlat=hist(nsecs-@s[args->next_pid]); delete(@s[args->next_pid]) }'

# off-CPU 时间按进程统计
bpftrace -e 'tracepoint:sched:sched_switch { @off[args->prev_comm] = count() }'

# 上下文切换频率
bpftrace -e 'tracepoint:sched:sched_switch { @++ }'
```

### 内存

```bash
# malloc 大小分布
bpftrace -e 'uprobe:/lib/x86_64-linux-gnu/libc.so.6:malloc { @size = hist(arg0); }'

# 缺页异常按进程统计
bpftrace -e 'tracepoint:exceptions:page_fault_user { @[comm] = count(); }'

# kmalloc 调用频率
bpftrace -e 'kprobe:kmalloc { @[comm] = count(); }'
```

### 文件 I/O

```bash
# 文件打开按进程统计
bpftrace -e 'tracepoint:syscalls:sys_enter_openat { @[comm] = count(); }'

# read() 返回值分布
bpftrace -e 'tracepoint:syscalls:sys_exit_read { @bytes = hist(args->ret); }'

# VFS 延迟直方图
bpftrace -e 'kprobe:vfs_read { @s[tid]=nsecs } kretprobe:vfs_read /@s[tid]/ { @lat=hist(nsecs-@s[tid]); delete(@s[tid]) }'
```

### 网络

```bash
# TCP 重传统计
bpftrace -e 'tracepoint:tcp:tcp_retransmit_skb { @[ntop(args->saddr),ntop(args->daddr)] = count() }'

# 连接建立延迟
bpftrace -e 'kprobe:tcp_v4_connect { @s[tid]=nsecs } kretprobe:tcp_v4_connect /@s[tid]/ { @lat=hist(nsecs-@s[tid]) }'

# sendto 字节数按进程
bpftrace -e 'tracepoint:syscalls:sys_enter_sendto { @[comm] = sum(args->len); }'

# 网卡发送延迟（qdisc → driver）
bpftrace -e 'kprobe:dev_queue_xmit { @s[tid]=nsecs } kprobe:dev_hard_start_xmit /@s[tid]/ { @drv=hist(nsecs-@s[tid]); delete(@s[tid]) }'
```

### 锁

```bash
# 互斥锁等待时间直方图
bpftrace -e 'kprobe:mutex_lock { @s[tid]=nsecs } kretprobe:mutex_lock /@s[tid]/ { @lock=hist(nsecs-@s[tid]); delete(@s[tid]) }'

# 按函数统计锁争用
bpftrace -e 'kprobe:__mutex_lock_slowpath { @[kstack] = count(); }'
```

## HFT 60 秒快速排障清单

```bash
# 1. 调度延迟（3 秒采样）
timeout 3 bpftrace -e 'tracepoint:sched:sched_wakeup { @s[tid]=nsecs } tracepoint:sched:sched_switch /@s[args->next_pid]/ { @rl=hist(nsecs-@s[args->next_pid]); delete(@s[args->next_pid]) }'

# 2. TCP 重传（10 秒采样）
timeout 10 bpftrace -e 'tracepoint:tcp:tcp_retransmit_skb { printf("%s %s:%d > %s:%d\n", strftime("%H:%M:%S"), ntop(args->saddr), args->sport, ntop(args->daddr), args->dport); @++ }'

# 3. on-CPU 热点（5 秒采样 → 火焰图）
timeout 5 bpftrace -e 'profile:hz:99 { @[kstack] = count() }'

# 4. 短命进程（持续监控）
bpftrace -e 'tracepoint:sched:sched_process_exec { printf("%s %s\n", strftime("%H:%M:%S"), comm) }'

# 5. 系统调用频率（5 秒采样）
timeout 5 bpftrace -e 'tracepoint:raw_syscalls:sys_enter { @[comm] = count() }'
```

### 常见陷阱

1. **直接复制 one-liner 不修改参数** — one-liner 中的函数名、PID、端口是示例值，需替换为目标环境实际值；不改参数可能匹配不到任何事件
2. **在高频事件上用 printf 逐行打印** — printf 每次调用都走 ring buffer 到用户态，高频 probe 上会导致输出爆炸+系统减速；应改用 Map 聚合（count/sum/hist）
3. **忽视 one-liner 的运行时长控制** — 某些 one-liner（如全系统 syscall 追踪）数据量巨大；生产环境应加 `timeout N` 或按进程过滤 `/comm == "myapp"/`

<details>
<summary>📝 自测题（点击展开）</summary>

1. **为什么 one-liner 中用 `@[kstack] = count()` 而非 `printf("%s", kstack)` 做火焰图数据？**

   <details>
   <summary>参考答案</summary>

   printf 每次命中都把完整栈字符串送到用户态，高频采样下 ring buffer 溢出+开销大。`@[kstack] = count()` 在内核 Map 中用栈 ID 聚合，只存栈哈希+计数，退出时一次性输出。开销低几个数量级，适合 99Hz 采样。
   </details>

2. **如何用 one-liner 快速判断 HFT 延迟尖刺是调度问题还是网络问题？**

   <details>
   <summary>参考答案</summary>

   两条 one-liner 并行短跑：(1) runqlat 直方图——如果尾部有毫秒级异常，说明调度抖动；(2) tcpretrans 计数——如果有重传事件，说明网络丢包。两者都正常则检查 offcputime（锁/IO 等待）和 irq 频率（中断干扰）。
   </details>

3. **malloc 大小分布的 one-liner `uprobe:libc:malloc { @size = hist(arg0); }` 在 HFT 上有什么风险？**

   <details>
   <summary>参考答案</summary>

   malloc 是极高频函数（每秒可能数万次），uprobe 每次 hit 都执行 BPF 程序+栈获取，per-hit 开销在微秒级。在 HFT 策略循环中 attach 此 probe 会显著放大延迟。应：(1) 短跑 1-2 秒采样；(2) 按进程过滤 `/comm == "myapp"/`；(3) 用 Map 聚合而非逐事件打印；(4) 绝不在生产交易时段运行。
   </details>

</details>

## 相关章节

- 上一章：[chapter-18-技巧与常见问题.md](./chapter-18-技巧与常见问题.md)
- 下一章：[appendix-B-bpftrace备忘单.md](./appendix-B-bpftrace备忘单.md)
