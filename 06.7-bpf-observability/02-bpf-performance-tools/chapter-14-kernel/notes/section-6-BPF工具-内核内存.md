# 6. BPF 工具：内核内存（kmem / kpages / memleak / slabratetop / numamove，14.4.7–14.4.11）

> 底本：《BPF之巅》第 14 章 内核，14.4.7–14.4.11 节（印刷 p688–693）

## 14.4.7 kmem

bpftrace 工具（作者 2019-03-15）：按调用栈统计**内核内存分配**（kmalloc/kmem_cache_alloc），打印**次数、平均大小、总字节**三元组：

```
# kmem.bt
@bytes[
    kmem_cache_alloc+288
    getname_flags+79
    getname+18
    do_sys_open+245
    SyS_openat+20
    Xorg]: count 44, average 4096, total 180224

@bytes[
    kmalloc_track_caller+368
    kmemdup+27
    intel_crtc_duplicate_state+37
    drm_atomic_get_crtc_state+119
    page_flip_common+51
    Xorg]: count 120, average 2048, total 245760
```

源代码：

```bash
tracepoint:kmem:kmalloc,
tracepoint:kmem:kmem_cache_alloc
{ @bytes[kstack(5), comm] = stats(args->bytes_alloc); }
```

- **stats()** 内置函数输出三元组；换 **hist()** 可得直方图
- 内存分配极频繁 → **繁忙系统开销较高**

## 14.4.8 kpages

bpftrace 工具：用 **kmem:mm_page_alloc** 跟踪点统计**页级分配**（alloc_pages()）的调用栈：

```
# kpages.bt
@pages[
    alloc_pages_nodemask+521
    alloc_pages_vma+136
    handle_pte_fault+959
    handle_mm_fault+1144
    ...
    chrome]: 11733        ← 缺页错误分配了 11733 页
```

```bash
tracepoint:kmem:mm_page_alloc
{ @pages[kstack(5), comm] = count(); }
```

本可写成单行，作者特意做成工具以防被忽略。开销同上（高）。

## 14.4.9 memleak

第 7 章介绍过的 BCC 工具，**默认跟踪内核内存分配**，显示跟踪期间**未被释放**的分配：

```
# memleak
[13:46:02] Top 10 stacks with outstanding allocations:
6922240 bytes in 1690 allocations from stack
    alloc_pages_nodemask+0x209 [kernel]
    page_cache_alloc / page_cache_get_page
    grab_cache_page_write_begin
    ext4_da_write_begin
    _generic_file_write_iter / ext4_file_write_iter
    new_sync_write / vfs_write
    sys_pwrite64 ...
```

定位内存增长或泄漏（此例是 ext4 写导致的正常页缓存分配）。详见第 7 章。

## 14.4.10 slabratetop

BCC + bpftrace 工具（作者 2016-10-15 BCC / 2019-01-26 bpftrace）：**kprobe kmem_cache_alloc()** 按 slab 缓存名显示**分配速率**，是 slabtop(1)（存量大小）的补充（流量）：

```
# slabratetop
09:48:29 loadavg: 6.30 5.45 5.46
CACHE          ALLOCS  BYTES
kmalloc-4096     654   2678784     ← 最多字节
kmalloc-256     2637    674816
filp             392    100352
sock_inode_cache  94    66176
TCP               31    63488
eventpoll_epi    227    45312
sigqueue         354    36320
dentry           165    31680
```

- kmem_cache_alloc 相对频繁 → 非常繁忙系统开销明显
- BCC 用法：`slabratetop [选项] [interval [count]]`；-C 不清屏

bpftrace 版要点——**按内核配置选头文件**：

```bash
#include <linux/mm.h>
#include <linux/slab.h>
#ifdef CONFIG_SLUB
#include <linux/slub_def.h>
#else
#include <linux/slab_def.h>
#endif

kprobe:kmem_cache_alloc
{
    $cachep = (struct kmem_cache *)arg0;
    @[str($cachep->name)] = count();
}
interval:s:1 { time(); print(@); clear(@); }
```

## 14.4.11 numamove

bpftrace 工具（作者 2019-01-26）：跟踪 **NUMA misplaced** 页迁移（迁往另一 NUMA 节点改善局部性）。动机：作者曾在生产系统遇到**高达 40% CPU 时间都在做 NUMA 页迁移**——损失超过了均衡收益，此工具盯防复发：

```
# numamove.bt
TIME      NUMA_migrations  NUMA_migrations_ms
22:48:45  0                 0
22:48:47  308               29        ← 迁移爆发：308 次 / 29ms
22:48:48  0                 0
```

```bash
kprobe:migrate_misplaced_page
{ @start[tid] = nsecs; }

kretprobe:migrate_misplaced_page /@start[tid]/
{
    $dur = nsecs - @start[tid];
    @ns += $dur; @num++;
    delete(@start[tid]);
}
interval:s:1
{ time(); printf("%18d %18d\n", @num, @ns/1000000); delete(@num); delete(@ns); }
```

前提：必须启用 NUMA 均衡（`sysctl kernel.numa_balancing=1`）。

## HFT 关联

- **slabratetop + slabtop = 流量+存量**：交易机上 TCP/dentry/skbuff 缓存的分配速率暴涨对应连接风暴或 fd 泄漏
- numamove 的 40% CPU 教训值得刻在墙上：多路服务器上跑交易进程，**先关 kernel.numa_balancing 或绑核绑内存**，否则页迁移可能吃掉惊人 CPU；用 numamove 验证
- kpages 的 mm_page_alloc 栈 = 第 7 章 faults 工具的内核视角，缺页来源分析两章互查

<details>
<summary>自测题</summary>

1. kmem 的 stats() 与 hist() 输出有何不同？
   <details><summary>答</summary>stats() 打印 count/average/total 三元组；hist() 打印分配大小的直方图分布。</details>

2. slabratetop 与 slabtop 的分工？
   <details><summary>答</summary>slabtop 显示缓存当前大小（存量，/proc/slabinfo）；slabratetop 显示每秒分配速率（流量，kprobe kmem_cache_alloc）。</details>

3. numamove 跟踪哪个函数？前提条件是什么？
   <details><summary>答</summary>kprobe/kretprobe migrate_misplaced_page；需要 sysctl kernel.numa_balancing=1。</details>

4. memleak 默认跟踪什么？
   <details><summary>答</summary>内核内存分配（Attaching to kernel allocators），显示未释放的分配栈；用户态需显式 -U 等选项（见第 7 章）。</details>
</details>
