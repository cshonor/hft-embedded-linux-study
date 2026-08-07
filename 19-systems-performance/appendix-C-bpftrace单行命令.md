# 附录 C bpftrace单行命令 · bpftrace One-Liners

> **Systems Performance 2nd** · Brendan Gregg · **精读**

> **定位：** SysPerf 附录 C 的 bpftrace 单行命令速查——出事时复制粘贴即跑。更完整的脚本集见 [20-bpf-observability/ref-bpftrace-scripts.md](../20-bpf-observability/ref-bpftrace-scripts.md)。

## CPU / 调度

```bash
# 调度延迟直方图（run queue 等待时长分布）
bpftrace -e 'tracepoint:sched:sched_wakeup { @start[tid] = nsecs; }
  tracepoint:sched:sched_switch /@start[args->prev_pid]/ { @runqlat = hist(nsecs - @start[args->prev_pid]); delete(@start[args->prev_pid]); }'

# off-CPU 时间 Top（哪个线程被切走后等最久）
bpftrace -e 'tracepoint:sched:sched_switch { @start[args->prev_pid] = nsecs; }
  tracepoint:sched:sched_wakeup /@start[args->pid]/ { @off[args->comm] = sum(nsecs - @start[args->pid]); }'

# 上下文切换计数（按进程）
bpftrace -e 'tracepoint:sched:sched_switch { @[args->prev_comm] = count(); }'
```

## 内存

```bash
#缺页异常计数（major vs minor）
bpftrace -e 'tracepoint:exceptions:page_fault_user { @[comm] = count(); }'

# direct reclaim 延迟
bpftrace -e 'kprobe:shrink_active_list { @start[tid] = nsecs; }
  kretprobe:shrink_active_list /@start[tid]/ { @reclaim = hist(nsecs - @start[tid]); delete(@start[tid]); }'

# kmalloc 分配大小分布
bpftrace -e 'tracepoint:kmem:kmalloc { @size = hist(args->bytes_alloc); }'
```

## 文件 / IO

```bash
# read syscall 延迟直方图
bpftrace -e 'tracepoint:syscalls:sys_enter_read { @start[tid] = nsecs; }
  tracepoint:syscalls:sys_exit_read /@start[tid]/ { @readlat = hist(nsecs - @start[tid]); delete(@start[tid]); }'

# open 路径追踪
bpftrace -e 'tracepoint:syscalls:sys_enter_openat { printf("%s %s\n", comm, str(args->filename)); }'

# block IO 延迟
bpftrace -e 'tracepoint:block:block_rq_issue { @start[args->dev, args->sector] = nsecs; }
  tracepoint:block:block_rq_complete /@start[args->dev, args->sector]/ { @iolat = hist(nsecs - @start[args->dev, args->sector]); delete(@start[args->dev, args->sector]); }'
```

## 网络

```bash
# TCP 重传事件
bpftrace -e 'tracepoint:tcp:tcp_retransmit_skb { @[comm] = count(); }'

# connect 延迟
bpftrace -e 'tracepoint:syscalls:sys_enter_connect { @start[tid] = nsecs; }
  tracepoint:syscalls:sys_exit_connect /@start[tid]/ { @connlat = hist(nsecs - @start[tid]); delete(@start[tid]); }'

# 软中断 NET_RX 耗时
bpftrace -e 'tracepoint:irq:softirq_entry /args->vec == 3/ { @start[cpu] = nsecs; }
  tracepoint:irq:softirq_exit /args->vec == 3 && @start[cpu]/ { @rx = hist(nsecs - @start[cpu]); delete(@start[cpu]); }'
```

## 锁

```bash
# futex 等待时长
bpftrace -e 'tracepoint:syscalls:sys_enter_futex { @start[tid] = nsecs; }
  tracepoint:syscalls:sys_exit_futex /@start[tid]/ { @futex = hist(nsecs - @start[tid]); delete(@start[tid]); }'
```

## HFT 生产使用原则

| 原则 | 说明 |
|------|------|
| **先 staging 后生产** | 自定义 kprobe 先在测试环境验证加载和开销 |
| **限时运行** | `timeout 10 bpftrace -e '...'`——避免遗忘导致长时间开销 |
| **map 聚合优先** | 高频事件用 `hist()/count()/sum()` 聚合，不要每条 `printf` |
| **tracepoint 优先** | tracepoint 是稳定 ABI，kprobe 函数名可能随内核版本变更 |

### 常见陷阱

1. **高频事件每条 printf**——sched_switch 每秒上千次，每条送到用户态会打爆 CPU，应用 `hist()`/`count()` map 聚合
2. **生产不限时长**——bpftrace 忘了 Ctrl-C 一直跑，应 `timeout 10 bpftrace ...` 兜底
3. **kprobe 函数名不验证**——内核升级后函数名可能变，加载失败但不报错（静默失败），应检查 `dmesg` 确认加载成功

<details>
<summary>自测题（点击展开）</summary>

1. bpftrace 单行命令中如何做延迟直方图？
   <details><summary>答</summary>enter 探针存 `@start[tid] = nsecs`，exit 探针算 `hist(nsecs - @start[tid])` 并 delete——log2 桶直方图显示延迟分布</details>
2. 高频事件为什么不能用 printf？
   <details><summary>答</summary>sched_switch 每秒上千次——每条送到用户态的 ring buffer 开销巨大，应用 map 聚合（hist/count/sum）只输出汇总</details>
3. 生产环境 bpftrace 的安全原则？
   <details><summary>答</summary>先 staging 验证加载和开销 → 生产 `timeout 10` 限时 → 优先 tracepoint 而非 kprobe → 检查 dmesg 确认加载成功</details>

</details>

## 相关章节

- 上一章：[appendix-B-sar总结.md](./appendix-B-sar总结.md)
- 下一章：[appendix-D-习题解答.md](./appendix-D-习题解答.md)
