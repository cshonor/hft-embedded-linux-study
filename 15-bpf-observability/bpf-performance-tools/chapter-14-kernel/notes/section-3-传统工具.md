# 3. 传统工具（14.3 节）

> 底本：《BPF之巅》第 14 章 内核，14.3 节（印刷 p670–675）

| 工具 | 类型 | 描述 |
|------|------|------|
| **Ftrace** | 跟踪 | Linux 内置的跟踪器 |
| **perf sched** | 跟踪 | Linux 官方剖析器的调度器分析子命令 |
| **slabtop** | 内核统计 | 内核 slab 缓存使用情况 |
| /proc/lockstat | 统计 | 内核锁统计（需 CONFIG_LOCK_STAT） |
| /proc/sched_debug | 统计 | 调度器开发辅助指标 |

## 14.3.1 Ftrace

Steven Rostedt 开发，2008 年进入 Linux 2.6.27。四种使用方法：

1. `/sys/kernel/debug/tracing` 文件 + cat/echo（文档：Documentation/trace/ftrace.rst）
2. **trace-cmd**(1)（Rostedt 的前端）
3. **KernelShark** GUI
4. 作者的 **perf-tools** 工具集（debugfs 的 shell 脚本封装）

### 函数调用统计（funccount）

分析文件系统 read-ahead 的例子——统计所有含 "readahead" 的函数：

```
# funccount '*readahead*'
FUNC                              COUNT
page_cache_async_readahead        12
__do_page_cache_readahead         33
page_cache_sync_readahead         69
ondemand_readahead                81
do_page_cache_readahead           83
```

### 调用栈（kprobe -Hs）

对上一步的函数抓栈，显示**父函数**（为什么被调用）：

```
# kprobe -Hs 'p:page_cache_async_readahead'
cksum-32372 [006] 1952191.125801: page_cache_async_readahead: (...)
cksum-32372 [006] 1952191.125822: <stack trace>
 => page_cache_async_readahead
 => ext4_file_read_iter
 => new_sync_read
 => vfs_read
 => ksys_read / SyS_read
 => do_syscall_64
 => entry_SYSCALL_64_after_hwframe
```

函数是在 read() 系统调用中被触发的。kprobe(8) 还能显示参数和返回值。

### hist triggers（内核上下文聚合）

逐事件打印栈效率低，用 Ftrace 的**直方图触发器**在内核上下文按栈聚合频率：

```
# cd /sys/kernel/debug/tracing/
# echo 'p:kprobes/myprobe page_cache_async_readahead' > kprobe_events
# echo "hist:key=stacktrace" > events/kprobes/myprobe/trigger
# cat events/kprobes/myprobe/hist
{ stacktrace:
  page_cache_async_readahead+0x5/0x80
  generic_file_read_iter+0x784/0xbf0
  ...
} hitcount: 235
```

### 函数调用图（funcgraph）

显示被调用的子函数与各级耗时：

```
# funcgraph page_cache_async_readahead
page_cache_async_readahead() {
    inode_congested() { dm_any_congested() { ... 0.582 us; } }
    ...
    ondemand_readahead() {
        __do_page_cache_readahead() {
            page_cache_alloc() { alloc_pages_current() { 0.234 us; } }
            ...
```

这些子函数可继续跟踪拿参数与返回值。

## 14.3.2 perf sched

perf(1) 的调度器分析子命令：

```
# perf sched record
# perf sched timehist
```

输出每个调度事件的四项时间指标：**阻塞时间（wait time）、等待唤醒时间、调度延迟 sch delay（即运行队列延迟）、on-CPU 运行时间 run time**。

## 14.3.3 slabtop

显示内核 slab 分配缓存当前大小（读 /proc/slabinfo），大生产系统按大小排序（-s c）：

```
# slabtop -s c
 OBJS ACTIVE  USE OBJ SIZE  SLABS  CACHE SIZE NAME
 76412  69196   0%     0.57K   2729     43664K radix_tree_node
313599 313599 100%     0.10K   8041     32164K buffer_head
  3732   3717   93%     7.44K    933     29856K task_struct
 11776    736   6%     2.00K    736     23552K TCP
 86100  79990   0%     0.19K   2050     16400K dentry
```

radix_tree_node 约 43MB、TCP 约 23MB——对 180GB 内存的系统很小。**定位内存压力**：检测某些内核组件是否意外占用大量内存。

## 14.3.4 其他

- `/proc/lockstat`：内核锁各种统计，需 CONFIG_LOCK_STAT
- `/proc/sched_debug`：调度器开发辅助指标

## HFT 关联

- Ftrace 的 funccount → kprobe 栈 → hist triggers → funcgraph 四连，是**无 BPF 环境**（老内核、受限容器）下同等的内核排查路径；hist triggers 思想与 BPF map 聚合一脉相承
- slabtop 是最先看的"内核吃了多少内存"快照：交易机上 task_struct（每线程 7.44K）× 万级线程就是几十 MB，线程数失控一眼可见

<details>
<summary>自测题</summary>

1. Ftrace 的四种使用方式？
   <details><summary>答</summary>debugfs 文件直接 echo/cat；trace-cmd；KernelShark GUI；perf-tools shell 封装。</details>

2. hist triggers 相比逐事件打印调用栈的优势？
   <details><summary>答</summary>在内核上下文按栈聚合频率（hitcount），避免把所有事件转储到用户态再后处理。</details>

3. perf sched timehist 输出哪四项时间指标？
   <details><summary>答</summary>阻塞时间、等待唤醒时间、调度延迟（运行队列延迟）、on-CPU 运行时间。</details>
</details>
