# 8. BPF 单行程序与挑战（14.5–14.7）

> 底本：《BPF之巅》第 14 章 内核，14.5–14.7 节（印刷 p697–699）

## 14.5.1 BCC 单行

```
syscount -p                                   # 按进程对系统调用计数
syscount                                      # 按系统调用名称计数
funccount 'attach*'                           # 对 attach 开头的内核函数计数
funclatency vfs_read                          # vfs_read() 延迟直方图
argdist -C 't:p:func1(int a):int:a'           # func1 第一个 int 参数出现频率
argdist -C 'r::func1():int:$retval'           # func1 返回值出现频率
argdist -C 'p::func1(struct sk_buff *skb):u32:skb->len'   # 强转 sk_buff 数 len 成员
profile -K -F 99                              # 99Hz 内核态栈采样
stackcount -p 123 t:sched:sched_switch        # 上下文切换调用栈计数
```

## 14.5.2 bpftrace 单行

```bash
# 按进程对系统调用计数
bpftrace -e 'tracepoint:raw_syscalls:sys_enter { @[pid, comm] = count(); }'

# 按系统调用探针名计数
bpftrace -e 'tracepoint:syscalls:sys_enter_* { @[probe] = count(); }'

# 按系统调用函数计数（syscall_table 反查）
bpftrace -e 'tracepoint:raw_syscalls:sys_enter { @[ksym(*(kaddr("syscall_table") + args->id*8))] = count(); }'

# vfs_read() 延迟直方图（双探针模板）
bpftrace -e 'k:vfs_read { @ts[tid] = nsecs; } kr:vfs_read /@ts[tid]/ { @hist(nsecs - @ts[tid]); delete(@ts[tid]); }'

# 99Hz 内核栈采样（排除 idle）
bpftrace -e 'profile:hz:99 /pid != 0/ { @[kstack] = count(); }'

# 99Hz on-CPU 内核函数采样
bpftrace -e 'profile:hz:99 { @[kstack(1)] = count(); }'

# 上下文切换调用栈计数
bpftrace -e 't:sched:sched_switch { @[kstack, ustack, comm] = count(); }'

# 按内核函数对工作队列请求计数
bpftrace -e 't:workqueue:workqueue_execute_start { @[ksym(args->function)] = count(); }'

# 对内核函数启动的 hrtimer 计数
bpftrace -e 't:timer:hrtimer_start { @[ksym(args->function)] = count(); }'
```

## 14.6 单行程序示例（带输出）

### 14.6.1 按系统调用函数对系统调用计数

```
# bpftrace -e 'tracepoint:raw_syscalls:sys_enter {
    @[ksym(*(kaddr("syscall_table") + args->id*8))] = count(); }'
@[sys_writev]:      5214
@[sys_sendto]:      5515
@[sys_read]:        6047
@[sys_epoll_wait]: 13232
@[sys_poll]:       15275
@[sys_ioctl]:      19010
@[sys_futex]:      20383
@[sys_write]:      26907
@[sys_gettid]:     27254
@[sys_recvmsg]:    51683        ← 最多
```

- 用**单个** raw_syscalls:sys_enter 跟踪点而非匹配全部 syscalls:sys_enter_*——**初始化和终止更快**
- raw_syscall 只给 ID → 通过 **syscall_table[id]** 反查函数指针，ksym() 翻译（同第 6/11 章技巧）

### 14.6.2 对内核函数启动的 hrtimer 计数

```
# bpftrace -e 't:timer:hrtimer_start { @[ksym(args->function)] = count(); }'
@[timerfd_tmrproc]:           2
@[watchdog_timer_fn]:         8
@[it_real_fn]:               78
@[perf_swevent_hrtimer]:     352     ← perf 正在跑软件事件剖析
@[hrtimer_wakeup]:          6156
@[tick_sched_timer]:       13514
```

作者开发此单行用于**检查 perf(1) 用的 CPU 剖析模式**（CPU 时钟 vs 周期事件）——软件剖析版本使用计时器（perf_swevent_hrtimer 出现即软件模式）。

## 14.7 挑战（跟踪内核函数的三大挑战）

| 挑战 | 说明 | 对策 |
|------|------|------|
| **编译器内联** | 内联函数对 BPF 跟踪不可见 | 跟踪未内联的、完成同一任务的父/子函数（可能需过滤）；或用 **kprobe 指令偏移量** |
| **黑名单函数** | 特殊模式（禁中断、跟踪框架自身）下跟踪不安全 | 内核黑名单机制使其无法被跟踪，只能换函数 |
| **kprobe 脆弱性** | 任何 kprobe 工具都需随内核改动维护——一些 BCC 工具已经损坏待修 | **长期方案：尽可能用跟踪点** |

## HFT 关联

- syscall_table 反查单行是"轻量 syscount"：一个探针看全部系统调用分布，交易机上秒级体检
- hrtimer 单行可查定时器风暴：策略里大量 timerfd/自制定时回调会以 hrtimer_wakeup 计数暴露
- 三大挑战里**内联**最常坑人：内核函数 kprobe 挂不上先怀疑被内联（尤其 static inline 的小函数），换父函数或偏移跟踪

<details>
<summary>自测题</summary>

1. 为什么用 raw_syscalls:sys_enter 比通配所有 syscalls:sys_enter_* 更好？
   <details><summary>答</summary>单个探针初始化/终止更快；代价是只有 ID，需 syscall_table 反查函数名。</details>

2. 内联函数跟踪不到的两种解决方法？
   <details><summary>答</summary>跟踪完成同一任务的未内联父函数或子函数（加过滤）；或用 kprobe 指令偏移量。</details>

3. 为什么作者说跟踪点是长期方案？
   <details><summary>答</summary>kprobe 依赖具体内核函数符号与行为，内核一改就坏（BCC 已有工具损坏案例）；跟踪点是稳定 ABI。</details>
</details>
