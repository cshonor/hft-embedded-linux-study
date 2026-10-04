# 5. BPF 工具：内核锁（mlock / mheld / 自旋锁，14.4.5–14.4.6）

> 底本：《BPF之巅》第 14 章 内核，14.4.5–14.4.6 节（印刷 p683–688）

## 14.4.5 mlock 和 mheld

与第 13 章 pmlock/pmheld 对应的**内核版**（作者 2019-03-14，同受 Solaris lockstat(1M) 启发）：直方图跟踪**内核互斥锁**延迟/持有时间 + 内核态栈。

### mlock——定位锁争用

```
# mlock.bt
@lock_latency_ns[0xffff9d015738c6e0,        ← 锁地址
    unix_stream_recvmsg+81
    sock_recvmsg+67
    __sys_recvmsg+245
    sys_recvmsg+81
    chrome]:                                ← 进程名
[512, 1K)   5859
[1K, 2K)    8303                            ← 8303 次在 1~2us：快
[2K, 4K)    1689
[4K, 8K)    476
[8K, 16K)   101
```

该锁被获取数千次但都很快（1~2us 档 8303 次）。

### mheld——找持有原因

```
# mheld.bt
@held_time_ns[0xffff9d015738c6e0,
    mutex_unlock+1
    unix_stream_recvmsg+81 ...              ← 持有者栈
    chrome]:
[1K, 2K)    74271
```

持有者与获取者是同一进程路径。

### 源代码与实现

```bash
# mlock 核心逻辑
kprobe:mutex_lock, kprobe:mutex_lock_interruptible
{ @lock_start[tid] = nsecs; @lock_addr[tid] = arg0; }

kretprobe:mutex_lock
/@lock_start[tid]/
{
    @lock_latency_ns[ksym(@lock_addr[tid]), kstack(5), comm]
        = hist(nsecs - @lock_start[tid]);
    ...
}
```

- mheld 额外跟踪 `mutex_trylock`（**kretprobe 过滤 retval == 0** 才算拿到锁）与 `mutex_unlock`；@held_start 以**锁地址**为键（内核中同一时刻一把锁只有一个持有者，不需要 pid 复合键——对比第 13 章 pmheld 的 [pid, 锁地址]）
- **mutex 跟踪点尚不存在**，用 kprobe 跟踪 mutex_lock()/mutex_lock_interruptible()/mutex_trylock() 内核函数
- 这些函数调用极频繁，**繁重负载下开销很高**；支持可选 PID 参数限定范围

## 14.4.6 自旋锁

**没有跟踪点，也不能用 kretprobes**：

- 多种变体：spin_lock_bh()、spin_lock()、spin_lock_irq()、spin_lock_irqsave()（include/linux/spinlock.h 中是宏，展开到 raw_spin_lock_* / _raw_spin_lock_irqsave 等）
- 研究发现**对这些函数用 kretprobes 插桩可能导致死锁**，BCC 文档专门列了被禁止的 kretprobes；内核还有 NOKPROBES_SYMBOL 黑名单
- → **只有入口可 kprobe，测不了持续时间**

### 可行手段一：funccount 计数

```
# funccount '*spin_lock*'
raw_spin_lock_bh              7092
native_queued_spin_lock_slowpath  7227
raw_spin_lock_irq            261538
raw_spin_lock               1215218
raw_spin_lock_irqsave       1582755    ← 最频繁
```

### 可行手段二：stackcount 看上层函数

自旋函数本身测不了时长，可看**栈上一层可跟踪的函数**（stackcount kprobe 调用栈）。

### 可行手段三（作者常用）：CPU 剖析 + 火焰图

自旋锁**以消耗 CPU 的函数形式出现**——CPU 剖析直接显示自旋路径（见 14.2 分析策略第 3 步）。

## HFT 关联

- 内核锁争用在高频网卡（XDP 前的驱动路径）与共享 socket 锁上很典型：mlock 直方图的 us 级尾部长度 = 内核路径对多核收发包的约束
- 自旋锁"测不了时长就剖析 CPU"的思路要记住：交易机上看到 `native_queued_spin_lock_slowpath` 出现在火焰图里，就是多核在内核锁上排队
- mheld 的 kretprobe retval==0 过滤与第 13 章 pmheld 同款，两章对照记忆

<details>
<summary>自测题</summary>

1. 为什么自旋锁无法像互斥锁那样测持续时间？
   <details><summary>答</summary>spin_lock 系列函数用 kretprobes 插桩可能导致系统死锁（BCC 有禁止清单），只能 kprobe 入口计数；测时长改用 CPU 剖析（自旋以耗 CPU 的函数形式出现）。</details>

2. mheld 的 @held_start 以什么为键？与第 13 章 pmheld 有何不同？
   <details><summary>答</summary>仅锁地址；内核互斥锁同一时刻只有一个持有者。pmheld 是 [pid, 锁地址]——用户态不同线程可同时持有不同锁。</details>

3. mlock/mheld 跟踪哪些内核函数？开销如何？
   <details><summary>答</summary>kprobe mutex_lock、mutex_lock_interruptible（mheld 加 mutex_trylock 与 mutex_unlock）；调用极频繁，繁重负载下开销很高，建议限定 PID。</details>
</details>
