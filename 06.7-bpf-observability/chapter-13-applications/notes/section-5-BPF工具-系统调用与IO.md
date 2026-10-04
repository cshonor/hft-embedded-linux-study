# 5. BPF 工具：系统调用与 I/O（syscount / ioprofile，13.2.7–13.2.8）

> 底本：《BPF之巅》第 13 章 应用程序，13.2.7–13.2.8 节（印刷 p641–644）

## 13.2.7 syscount

BCC 的系统调用计数工具（Sasha Goldshtein 2017 年版；作者 2014 年 perf(1) 版是 strace -c 的轻量替代），按类型展示**应用程序的资源使用视图**。MySQL 上每秒输出（-i 1）：

```
# syscount -i 1 -d 10 -p $(pgrep mysqld)
[11:49:25]
SYSCALL      COUNT
sched_yield  10848
recvfrom     6576
futex        3977
sendto       2193
poll         2187
pwrite       128
fsync        115
nanosleep    ...
```

sched_yield 每秒超一万次最频繁。可配合 syscall 跟踪点深挖：BCC `stackcount(8)` 显示导致该调用的调用栈，`argdist(8)` 总结参数。每个系统调用都有 man 手册页。

### -L：按总耗时排序（找优化目标）

```
# syscount -L -m -d 10 -p $(pgrep mysqld)
SYSCALL      COUNT   TIME(ms)
futex        42158   108139.6    ← 10 秒窗口内总耗时 108 秒（多线程叠加）
nanosleep    9000    992.9
fsync        1176    4393.4      ← 最有意思：优化目标 → 文件系统/存储设备
poll         22700   1237.2
sendto       22795   276.4
recvfrom     68311   275.9
sched_yield  79759   141.3
```

- **futex 108 秒**：多线程同时调用的时间叠加；频繁调用很可能是"等待工作"机制（与 offcputime 的发现一致）
- **fsync 4393ms**：从上往下最有意思的目标——指向文件系统和存储设备优化

## 13.2.8 ioprofile

作者 2019-02-15 为本书开发。跟踪 **I/O 相关系统调用——读、写、发送、接收——按用户态调用栈统计次数**（原计划加入 Netflix Vector 软件包）：

```
# ioprofile.bt $(pgrep mysqld)
Attaching 24 probes....
@[tracepoint:syscalls:sys_enter_pwrite64,
    pwrite+114
    os_file_io(IoRequest const&, int, void*, ...)
    os_file_write_page(...)
    fil_io(...)
    log_write_up_to(...)                ← 事务日志写
    trx_commit_complete_for_mysql(trx_t*)
    innobase_commit(handlerton*, THD*, bool)
    ...
    trans_commit(THD*)
    mysql_execute_command(THD*, bool)
    ...
    dispatch_command(...)
    do_command(THD*)
    handle_connection
    mysqld]: 636

@[tracepoint:syscalls:sys_enter_recvfrom,
    recv+152
    vio_read
    net_read_packet(st_net*, unsigned long*)
    my_net_read
    Protocol_classic::get_command(COM_DATA*, ...)
    do_command(THD*)                    ← 读客户端数据包
    handle_connection
    mysqld]: 24255
```

用途：**应用做了太多/不必要的 I/O** 是常见性能问题——可关掉的写日志、可增大的 I/O 大小等，此工具定位到代码路径。

源代码（24 个通配探针）：

```bash
#!/usr/local/bin/bpftrace
BEGIN { printf("Tracing I/O syscall user stacks. Ctrl-c to end.\n"); }

tracepoint:syscalls:sys_enter_*read*,
tracepoint:syscalls:sys_enter_*write*,
tracepoint:syscalls:sys_enter_*send*,
tracepoint:syscalls:sys_enter_*recv*
{ @[probe, ustack, comm] = count(); }
```

- 可选位置参数 PID；不提供则跟踪整个系统
- **开销**：这些是高频系统调用，性能损失明显
- 本书写它的另一个动机：展示 libc/libpthread 没有帧指针有多痛苦（→ 下一节 13.2.9）

## HFT 关联

- `syscount -L -m -p <策略PID>` 是一分钟的快速体检：futex 高 = 锁/等待问题；sendto/recvfrom 高 = 网络路径；fsync 高 = 落盘路径（持久化订单流）
- ioprofile 的"计数版"思想可复制到交易日志写路径：把 pwrite64 的 ustack 折叠成火焰图，检查是否有冗余写（如每笔订单多次小写入 → 改批量/大块写）

<details>
<summary>自测题</summary>

1. syscount 哪个选项显示系统调用总耗时、哪个选项以毫秒为单位？
   <details><summary>答</summary>-L 显示总时间，-m 以毫秒汇总。</details>

2. 为什么 10 秒跟踪里 futex 能显示 108 秒总耗时？
   <details><summary>答</summary>总时间是多线程同时阻塞在 futex 的叠加，不是墙上时钟时间。</details>

3. ioprofile 跟踪哪些系统调用、用什么探针？
   <details><summary>答</summary>sys_enter_*read*/*write*/*send*/*recv* 跟踪点（通配，共 24 个探针），按 ustack 计数。</details>
</details>
