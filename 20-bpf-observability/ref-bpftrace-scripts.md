# bpftrace 样例脚本集合 — 19 SysPerf 症状 → 20 BPF 钻取

> 每个脚本标注：**触发场景**（19 看到的现象）→ **脚本** → **输出解读** → **HFT 注意**。
> 所有脚本默认 `sudo` 运行；生产环境务必加 `timeout` 和 PID 过滤。

---

## 1. 调度延迟 — runqlat 手写版

**触发：** `vmstat 1` 的 `r` 列 > CPU 核数；或 `runqlat-bpfcc` 右尾拉长。

```bash
# 测量线程从入队 RUNNABLE 到真正上 CPU 运行的等待时间
# 用 tracepoint（稳定接口），不依赖 kprobe 函数名
sudo timeout 10 bpftrace -e '
tracepoint:sched:sched_wakeup,
tracepoint:sched:sched_wakeup_new {
    @q[tid] = nsecs;
}
tracepoint:sched:sched_switch {
    if (@q[args->next_pid] != 0) {
        @runq_lat = hist((nsecs - @q[args->next_pid]) / 1000);  // us
        delete(@q[args->next_pid]);
    }
}
interval:s:10 { exit(); }
'
```

**输出解读：**

```
@runq_lat:
[4, 8)               152 |                                                    |
[8, 16)             3201 |████████████████████                                |
[16, 32)            8432 |████████████████████████████████████████████████████|
[32, 64)             891 |█████                                               |
[64, 128)             23 |                                                    |
[128, 256)             2 |                                                    |
```

- 主峰在 8–32us → 健康（绑核 dedicated 核应在此范围）
- 右尾到 64us+ → 有排队，检查邻居进程 / cgroup 配额
- 到 ms 级 → 严重饱和，热路径不可接受

**HFT：** 绑核核对时加 `/args->next_pid == <交易线程PID>/` 过滤，避免噪声。

---

## 2. 块 I/O 完整链路延迟 — biolatency 手写版

**触发：** `iostat -x 1` 的 `await` 上涨；或 `biolatency-bpfcc` 双峰/右尾。

```bash
# 追踪块 I/O 从提交到完成的完整延迟
# tracepoint:block:block_rq_issue → 发起请求
# tracepoint:block:block_rq_complete → 设备完成
sudo timeout 10 bpftrace -e '
tracepoint:block:block_rq_issue {
    @start[args->dev, args->sector] = nsecs;
}
tracepoint:block:block_rq_complete /@start[args->dev, args->sector]/ {
    @iolat = hist((nsecs - @start[args->dev, args->sector]) / 1000);  // us
    delete(@start[args->dev, args->sector]);
}
interval:s:10 { exit(); }
'
```

**输出解读：**

| 形态 | 含义 | 下一步 |
|------|------|--------|
| 单峰 < 100us | 命中设备 cache | 健康 |
| 双峰 (100us + ms) | 部分走介质 | 检查预读 / direct-io |
| 整体右移 > 1ms | 盘饱和或调度拥塞 | 查 IOPS、队列深度 |
| 右尾突刺到 10ms+ | 长尾 outlier | 共置机噪声 / GC 停顿 |

**HFT：** 热路径不应触发块 I/O。若看到输出 → 检查 mmap 是否被换出、是否有异步日志写盘。

---

## 3. TCP 重传 + 建连延迟

**触发：** `netstat -s` 的 retransmits 增长；或 `tcpretrans-bpfcc` 有输出。

```bash
# 3a. TCP 重传事件（逐条打印）
sudo timeout 30 bpftrace -e '
tracepoint:tcp:tcp_retransmit_skb {
    printf("%-8s pid=%-6s %-6s -> ", strftime("%H:%M:%S"), pid, comm);
    printf("%s:%d > %s\n", ntop(args->saddr), args->sport, ntop(args->daddr));
}
'

# 3b. TCP connect() 延迟分布（谁是慢连接元凶）
sudo timeout 10 bpftrace -e '
tracepoint:syscalls:sys_enter_connect /comm == "your_app"/ {
    @connect_start[tid] = nsecs;
}
tracepoint:syscalls:sys_exit_connect /@connect_start[tid]/ {
    @connect_lat = hist((nsecs - @connect_start[tid]) / 1000000);  // ms
    delete(@connect_start[tid]);
}
interval:s:10 { exit(); }
'
```

**输出解读：**

- 3a：如果重传集中在某目的 IP → 对端慢或网络抖动
- 3a：如果重传集中在 SYN → backlog 溢出或 SYN flood
- 3b：connect 延迟 P99 > 1ms → 内网不正常（应 < 500us）

**HFT：** 行情连接重传 = 丢包 = 序列号断裂。看到任何 retransmit 都要查。

---

## 4. Syscall 延迟分布 — open/read/write/fsync

**触发：** `perf top` 显示 sys_call 入口占比高；或业务延迟 histogram 有 IO 段突起。

```bash
# 4a. 通用版：追踪指定 PID 的 read() 延迟
sudo timeout 10 bpftrace -e '
tracepoint:syscalls:sys_enter_read /pid == 12345/ {
    @read_start[tid] = nsecs;
}
tracepoint:syscalls:sys_exit_read /@read_start[tid]/ {
    @read_lat = hist((nsecs - @read_start[tid]) / 1000);  // us
    delete(@read_start[tid]);
}
interval:s:10 { exit(); }
'

# 4b. fsync 延迟（写盘确认 — 通常是最慢的 syscall）
sudo timeout 10 bpftrace -e '
tracepoint:syscalls:sys_enter_fsync {
    @fsync_start[tid] = nsecs;
}
tracepoint:syscalls:sys_exit_fsync /@fsync_start[tid]/ {
    @fsync_lat = hist((nsecs - @fsync_start[tid]) / 1000000);  // ms
    printf("fsync: pid=%d comm=%s lat=%dms\n", pid, comm,
           (nsecs - @fsync_start[tid]) / 1000000);
    delete(@fsync_start[tid]);
}
interval:s:10 { exit(); }
'
```

**输出解读：**

| syscall | 健康范围 | 异常 |
|---------|----------|------|
| read (cached) | < 10us | > 100us → 可能触发块 I/O |
| read (direct) | 取决于设备 | 双峰 → 部分走介质 |
| write (buffered) | < 20us | > 1ms → page cache 压力 |
| fsync | 取决于设备 | > 10ms → 盘饱和或 journal 瓶颈 |
| connect | < 500us | > 1ms → 网络栈 / backlog 问题 |

**HFT：** 用 4a 的模式替换 read 为 openat/futex/epoll_wait，覆盖热路径全部 syscall。

---

## 5. 锁竞争 — futex 等待时长

**触发：** `perf top` 显示 `__futex_wait` 或 `futex_lock_pi` 占比高；CPU 不高但延迟大。

```bash
# 追踪 futex 系统调用的等待时长（用户态锁的内核侧表现）
sudo timeout 10 bpftrace -e '
tracepoint:syscalls:sys_enter_futex /args->op == 0/ {  // FUTEX_WAIT
    @futex_start[tid] = nsecs;
}
tracepoint:syscalls:sys_exit_futex /@futex_start[tid]/ {
    @futex_lat = hist((nsecs - @futex_start[tid]) / 1000);  // us
    delete(@futex_start[tid]);
}
interval:s:10 { exit(); }
'
```

**输出解读：**

- 主峰 < 10us → 自旋锁快速获取，正常
- 峰在 100us–1ms → 有真实竞争，多线程在等同一把锁
- 右尾到 ms 级 → 持锁者做了慢操作（IO / 长计算），需拆锁

**HFT：** 无锁队列不应出现 futex 等待。如果看到 → 检查是否误用了 mutex/spinlock 在热路径。

---

## 6. 缺页异常 — major/minor fault 热点

**触发：** `perf stat` 的 `faults` 高；或启动后仍有 major fault。

```bash
# 6a. 区分 minor / major page fault 的频率
sudo timeout 10 bpftrace -e '
tracepoint:exceptions:page_fault_user {
    @minor[comm] = count();
}
tracepoint:exceptions:page_fault_kernel {
    @major[comm] = count();
}
interval:s:10 { exit(); }
'

# 6b. major fault 的栈回溯（定位谁触发了磁盘读）
sudo timeout 10 bpftrace -e '
tracepoint:exceptions:page_fault_kernel {
    @[kstack] = count();
}
interval:s:10 { exit(); }
' | head -30
```

**输出解读：**

| 类型 | 含义 | HFT 要求 |
|------|------|----------|
| minor fault | 页已在内存（page cache / COW） | 启动后可接受少量 |
| major fault | 需要从磁盘读页 | **热路径 = 0** |

**HFT：** 启动后 `mlockall()` + 预热。major fault > 0 → 检查 mlock 是否生效、是否有新 mmap 未预读。

---

## 7. 内存分配热点 — slab / page alloc

**触发：** `slabtop` 显示某 cache 膨胀；或 `/proc/meminfo` 的 Slab 持续增长。

```bash
# 追踪 kmalloc 调用频次和大小分布（kprobe — 注明内核版本依赖）
sudo timeout 10 bpftrace -e '
kprobe:__kmalloc {
    @kmalloc_size = hist(arg1);  // arg1 = size
    @kmalloc_caller[kstack] = count();
}
interval:s:10 { exit(); }
'
```

**输出解读：**

- `@kmalloc_size` 看分布 → 大量小对象分配考虑 slab 池化
- `@kmalloc_caller` 排序 → 最热的调用栈就是分配源头

**HFT：** 热路径不应有 kmalloc。如果看到 → 检查是否在数据路径里做了动态分配（应预分配 / 池化）。

---

## 8. 软中断分布 — net_rx 耗时

**触发：** `mpstat -I SUM 1` 的 `%soft` 高；或网络包处理延迟大。

```bash
# 追踪 NET_RX 软中断的执行时长
sudo timeout 10 bpftrace -e '
tracepoint:irq:softirq_entry /args->vec == 3/ {  // NET_RX_SOFTIRQ = 3
    @netrx_start[cpu] = nsecs;
}
tracepoint:irq:softirq_exit /@netrx_start[cpu]/ {
    @netrx_lat = hist((nsecs - @netrx_start[cpu]) / 1000);  // us
    delete(@netrx_start[cpu]);
}
interval:s:10 { exit(); }
'
```

**输出解读：**

- P99 < 100us → 健康
- 右尾到 ms 级 → 单次 softirq 处理了太多包，考虑 RPS / busy polling
- 哪个 CPU 的 softirq 占比高 → 网卡中断绑核检查

**HFT：** 行情核和网卡中断核应分离。用 `/proc/interrupts` 确认中断不落在交易核。

---

## 脚本使用纪律（20 Ch18）

| 规则 | 说明 |
|------|------|
| **限时** | 所有脚本加 `timeout` 或 `interval:s:N { exit() }` |
| **限 PID** | 生产加 `/pid == XXXX/` 或 `/comm == "xxx"/` 过滤 |
| **限核** | 绑核场景加 `/cpu == N/` 过滤 |
| **不堆栈** | `kstack` 输出只在离线分析时用，生产用 `count()` / `hist()` |
| **先测** | 新脚本先在测试机跑通，确认无 verifier 拒绝 / 无 OOM |
| **内核版本** | kprobe 脚本注明 `uname -r`；升级内核后需重新验证 |
