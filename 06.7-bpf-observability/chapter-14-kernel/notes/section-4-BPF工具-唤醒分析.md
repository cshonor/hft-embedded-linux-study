# 4. BPF 工具：唤醒分析（loads / offcputime 深入 / wakeuptime / offwaketime，14.4.1–14.4.4）

> 底本：《BPF之巅》第 14 章 内核，14.4.1–14.4.4 节（印刷 p675–683）

## 14.4.1 loads

bpftrace 工具，每秒打印系统平均负载（作者 2005 DTrace loads.d → 2018 bpftrace）：

```
# loads.bt
18:49:16 load averages: 1.983 1.151 0.931
```

平均负载用处有限（第 6 章结论），此工具真正的价值是**演示读取和打印内核变量**——avenrun：

```bash
interval:s:1
{
    $avenrun = kaddr("avenrun");        // 内核符号地址
    $load1 = *$avenrun;                 // 定点数，需移位换算
    $load5 = *($avenrun + 8);
    $load15 = *($avenrun + 16);
    time("%H:%M:%S");
    printf("load averages: %d.%03d %d.%03d %d.%03d\n",
        $load1 >> 11, (($load1 & ((1<<11)-1)) * 1000) >> 11, ...);
}
```

**kaddr() 取符号地址再解引用**——任何内核变量都可用此法读取。

## 14.4.2 offcputime（深入：状态过滤与"没有定论的栈"）

第 6 章介绍过；本节看它的两个进阶用法。

### 不可中断 I/O（--state 2）

筛选 **TASK_UNINTERRUPTIBLE** 状态，展示应用阻塞在等待**资源**的时间：

```
# offcputime -uK --state 2
    finish_task_switch
    schedule
    io_schedule
    generic_file_read_iter
    xfs_file_buffered_aio_read
    ...
    ksys_read
    tar(7034)
    1088682
```

tar 通过 XFS 等存储 I/O。过滤掉的其他状态：

- **TASK_RUNNING(0)**：CPU 饱和导致的**被动上下文切换**——栈无意义，无法显示为什么被移出 CPU
- **TASK_INTERRUPTIBLE(1)**：大量"等待工作的睡眠"栈会**污染输出**

> TASK_UNINTERRUPTIBLE 时间**也计入 Linux 平均负载**——很多人以为平均负载只含 CPU 执行，这是误解之源（第 6 章）。

### 没有定论的栈（inconclusive stacks）

offcputime 的许多栈只展示"阻塞在哪"，**不展示原因**。gzip 进程 5 秒跟踪：

```
# offcputime -K -p $(pgrep -n gzip) 5
    finish_task_switch
    schedule
    pipe_wait          ← 只知道在等管道
    pipe_read
    vfs_read ...
    gzip(5028)
    4404219            ← 5 秒里 4.4 秒在 pipe_read
```

无法得知管道另一边是谁、为什么慢。这类栈很普遍（管道、I/O、锁争用都一样）——**用 wakeuptime(8) 检查唤醒栈**可揭示等待的另一边。

## 14.4.3 wakeuptime

BCC 工具：展示**执行唤醒的调用栈** + 目标被阻塞的时间（作者 2013 DTrace 版为 LISA 火焰图讲座而写，2016 BCC）。续上例：

```
# wakeuptime -p $(pgrep -n gzip) 5
target: gzip
    entry_SYSCALL_64_after_hwframe
    do_syscall_64
    ksys_write
    vfs_write ...
    pipe_write
    wakeup_common_lock
    wakeup_common
    autoremove_wake_function
waker: tar
4551336                                  ← tar 唤醒时 gzip 已阻塞 4.55 秒
```

**真相大白**：`tar cf - /mnt/data | gzip -> /mnt/backup.tar.gz`——gzip 大部分时间在等 tar 写管道；tar 大部分时间在等磁盘（offcputime 看 tar：阻塞在 io_schedule → 块设备 I/O）。

- offcputime 回答**为什么被阻塞**，wakeuptime 回答**被谁为什么唤醒**——**有时唤醒侧更能定位问题来源**
- 实现：kprobe `schedule()` + `try_to_wake_up()`——繁忙系统调用极频繁，**开销可能很高**
- 用法：`wakeuptime [选项] [时长]`；`-f` 折叠输出（火焰图）、`-p PID` 限定进程；不指定 -p 跟踪全系统会有数百页输出

## 14.4.4 offwaketime

BCC 工具，**结合** offcputime + wakeuptime：

```
# offwaketime -K -p $(pgrep -n gzip) 5
waker: tar 5852
    entry_SYSCALL_64_after_hwframe       ← 唤醒者栈（上方，已反转）
    ...
    pipe_write
    wakeup_common ...
    finish_task_switch
    schedule
    pipe_wait                            ← 阻塞栈（下方）
    pipe_read ...
target: gzip 5851
4490207
```

- 两个栈用 `--` 分隔，**唤醒者栈在上（反转），阻塞栈在下，在中间相遇**（唤醒发生处）
- 实现：跟踪 schedule() + try_to_wake_up()，**用 BPF 栈映射表保存唤醒者栈，被阻塞线程查询后合并**——两栈在内核上下文统一汇总。DTrace 时代只能转储全部事件后处理（生产开销不可接受），BPF 可存取栈且限制在一层唤醒（作者 2016-01-13 BCC 版；Alexei Starovoitov 的内核示例在 samples/bpf/offwaketime_*.c）
- 选项：-f 折叠、-p PID、-K 仅内核栈、-U 仅用户栈；用 -p/-K/-U **减少开销**
- **Off-Wake 时间火焰图**（图 14-4）：同样方向——唤醒者栈在上（含 I/O 完成中断路径 blk_mq_complete_request → bio_endio → unlock_page → wake_up），阻塞栈在下（tar 等 ext4 页锁），完整呈现"设备完成中断如何唤醒阻塞线程"的因果链

## HFT 关联

- offwaketime 是"系统卡但应用说不清"的终极武器：策略线程等管道/等套接字/等页锁，火焰图上直接看到**唤醒它的内核路径**（网卡驱动 → softirq → wake_up）
- --state 2 过滤是只看资源阻塞的快捷键；交易系统排障时先跑它剔除线程池 idle 噪声
- loads.bt 的 kaddr+解引用模式可用来读任何内核计数器（如内部延迟统计），自定义小工具的起点

<details>
<summary>自测题</summary>

1. offcputime --state 2 过滤的是什么状态？为什么 TASK_RUNNING 的栈无意义？
   <details><summary>答</summary>TASK_UNINTERRUPTIBLE（不可中断 I/O 等待）；TASK_RUNNING 是 CPU 饱和下的被动切换，栈只能显示切换点，无法说明为何被移出 CPU。</details>

2. wakeuptime 与 offcputime 各回答什么？什么情况下唤醒侧更有用？
   <details><summary>答</summary>offcputime 显示为什么阻塞（阻塞侧栈），wakeuptime 显示谁唤醒了它（唤醒侧栈）；当阻塞栈"没有定论"（如 pipe_read 看不到对端）时，唤醒栈能揭示等待的另一边。</details>

3. offwaketime 输出中两个栈如何排列？
   <details><summary>答</summary>唤醒者栈在上且已反转，阻塞者栈在下，两栈在中间"相遇"于唤醒点，用 -- 分隔。</details>
</details>
