# 4. BPF 工具：Off-CPU 剖析（offcputime / offcpuhist，13.2.5–13.2.6）

> 底本：《BPF之巅》第 13 章 应用程序，13.2.5–13.2.6 节（印刷 p634–641）

## 13.2.5 offcputime

第 6 章的 BCC 工具，跟踪线程**何时阻塞并离开 CPU**，用调用栈记录离开时长。MySQL 示例：

```
# offcputime -d -p $(pgrep mysqld)
    finish_task_switch
    schedule
    jbd2_log_wait_commit              ← 阻塞点
    jbd2_complete_transaction
    ext4_sync_file
    vfs_fsync_range
    do_fsync
    sys_fsync
    ...
    fsync
    fil_flush(unsigned long)
    log_write_up_to(...)               ← 用户态（MySQL）从这里开始
    trx_commit_complete_for_mysql(trx_t*)
    innobase_commit(handlerton*, THD*, bool)
    ha_commit_low(THD*, bool, bool)
    ...
    trans_commit(THD*)
    mysql_execute_command(THD*, bool)
    ...
    dispatch_command(THD*, COM_DATA const*, enum server_command)
    do_command(THD*)
    handle_connection
    pfs_spawn_thread
    start_thread
    mysqld(9962)
    2458362                            ← 2458362 us = 2.45 秒（所有线程总计）
```

**读栈方法**（第一个栈）：MySQL 语句 → 事务提交 → 写日志 → fsync() → 进入内核（ext4 处理 fsync）→ 最终阻塞在 **jbd2_log_wait_commit()**（ext4 日志线程）。

后两个栈：`lock_wait_timeout_thread()` 经 pthread_cond_timedwait() 等事件、`srv_master_thread()` 进入睡眠——**offcputime 输出经常被等待/睡眠线程占据，这通常正常而非问题**。你的任务是找**应用程序处理请求过程中阻塞的栈**，那才是问题所在。

### Off-CPU 时间火焰图

```
# offcputime -f -p $(pgrep mysqld) 10 > out.offcputime01.txt
# flamegraph.pl --countname=us < out.offcputime01.txt > out.offcputime01.svg
```

- 用火焰图**搜索功能**品红高亮包含 "do_command" 的帧——这些是 MySQL 处理请求的代码路径，**客户端就阻塞在这些路径上**（图 13-3 匹配仅 1.5%）
- 点击窄塔放大（图 13-4）：鼠标悬停 `ext4_sync_file()` 底部显示 **3.95 秒**——这是阻塞在 do_command() 下可优化的目标

**开销**：取决于上下文切换率，**可超过 5%**——可控：生产环境只跑很短时间。BPF 之前做 off-CPU 分析需把所有栈转储到用户态后处理，损耗大到通常禁止在生产使用。

## 13.2.6 offcpuhist

与 offcputime 类似（作者 2019-02-16 开发，源自 2011 年 DTrace 书的 uoffcpu.d），跟踪调度器事件记录 off-CPU 时间，但按**直方图**而非总数显示：

```
# offcpuhist.bt $(pgrep mysqld)
@[
    finish_task_switch+1
    schedule+44
    futex_wait_queue_me+196
    futex_wait+266
    do_futex+805
    ...
    pthread_cond_wait+432
    os_event::wait_low(os_event*, long)+64
    srv_worker_thread+503          ← 工作线程等活
    start_thread+208
    clone+63
    mysqld]:
[2K, 4K)     134
[4K, 8K)     293
[8K, 16K)    886            ← 模式一：约 16us（纳秒显示为 8K~16K）
[16K, 32K)   493
[32K, 64K)   447            ← 模式二
[4M, 8M)     306
[8M, 16M)    747            ← 模式三：8~16 毫秒（srv_worker 等待工作的双峰）
```

- 第一个栈：`srv_worker_thread()` 等工作的**双峰延迟分布**——16us 与 8–16ms 两种模式
- 第二个栈：`net_read_packet()` 路径上更短的等待，通常 <128us

源代码（kprobe 双时间戳法）：

```bash
#!/usr/local/bin/bpftrace
#include <linux/sched.h>
BEGIN { printf("Tracing nanosecond time in off-cpu stacks. Ctrl-c to end.\n"); }

kprobe:finish_task_switch
{
    // 记录前一个线程的睡眠开始时间
    $prev = (struct task_struct *)arg0;
    @start[$prev->pid] = nsecs;
    // 当前线程开始用 CPU 时，记录直方图
    $last = @start[tid];
    if ($last != 0) {
        @[kstack, ustack, comm, tid] = hist(nsecs - $last);
        delete(@start[tid]);
    }
}

END { clear(@start); }
```

- **kprobe:finish_task_switch**：每次调度都经过——离开 CPU 时记时间戳，回来时算差值入直方图
- **开销大**（kprobe 调度器热点），仅限短时间运行

## HFT 关联

- 策略延迟问题 = off-CPU 分析的主战场：订单线程阻塞在 `tcp_sendmsg`/`fsync`/futex 的栈，配合火焰图搜索策略入口函数名（如同书中搜 do_command）
- offcputime >5% 开销：交易时段慎用，先在演练/回放环境跑，生产只采样 1–2 秒
- offcpuhist 的**双峰分布**读法要熟练：us 级尖峰（锁自旋）vs ms 级尾部（IO/网络等待）对应完全不同的优化方向

<details>
<summary>自测题</summary>

1. offcputime 输出中大量等待线程的栈是问题吗？
   <details><summary>答</summary>通常不是——线程池等待工作、后台线程睡眠都正常。要找的是应用处理请求路径上的阻塞栈（如 do_command 之下）。</details>

2. offcpuhist 相对 offcputime 的差异是什么？
   <details><summary>答</summary>同样基于调度器事件记 off-CPU 时间，但按延迟直方图显示（暴露分布/多峰），offcputime 只给总时间。</details>

3. 这两个工具的开销量级与控制手段？
   <details><summary>答</summary>取决于上下文切换率，可超 5%（kprobe finish_task_switch 是调度热点）；控制手段是生产环境只运行很短时间。</details>
</details>
