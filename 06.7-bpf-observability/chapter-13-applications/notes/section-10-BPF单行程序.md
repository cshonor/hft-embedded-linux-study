# 10. BPF 单行程序（13.3–13.4）

> 底本：《BPF之巅》第 13 章 应用程序，13.3–13.4 节（印刷 p662–664）

## 13.3.1 BCC 单行

```
execsnoop                                        # 带参数的新创建进程
syscount -p                                      # 按进程对系统调用计数
syscount                                         # 按系统调用名称计数
profile -U -F 49 -p 189                          # PID 189 以 49Hz 采样用户态栈
stackcount -u t:sched:sched_switch               # 对 off-CPU 用户态调用栈计数
profile                                          # 对所有调用栈和进程名采样
funccount -d 1 /lib/x86_64-linux-gnu/libpthread.so.0:pthread_mutex*   # 互斥锁方法计数 1 秒
funccount -d 1 /lib/x86_64-linux-gnu/libpthread.so.0:pthread_cond_*   # 条件变量函数计数 1 秒
```

## 13.3.2 bpftrace 单行

```bash
# 带参数的新创建进程
bpftrace -e 'tracepoint:syscalls:sys_enter_execve { join(args->argv); }'

# 对 mysqld 进程以 49Hz 采样用户态调用栈
bpftrace -e 'profile:hz:49 /comm == "mysqld"/ { @[ustack] = count(); }'

# 对所有调用栈和进程名采样
bpftrace -e 'profile:hz:49 { @[ustack, kstack, comm] = count(); }'

# 按用户态调用栈计算 malloc() 请求的字节总数（高开销！）
bpftrace -e 'u:/lib/x86_64-linux-gnu/libc-2.27.so:malloc /@[ustack(5)] = sum(arg0);/'

# 跟踪 kill() 信号：发送进程名、目标 PID、信号号码
bpftrace -e 't:syscalls:sys_enter_kill { printf("%s -> PID %d SIG %d\n", comm, args->pid, args->sig); }'

# 对 libpthread 互斥锁/条件变量相关函数计数 1 秒
bpftrace -e 'u:/lib/x86_64-linux-gnu/libpthread.so.0:pthread_cond_* { @[probe] = count(); } interval:s:1 { exit(); }'

# 按进程对 LLC 缓存未命中计数
bpftrace -e 'hardware:cache-misses { @[comm] = count(); }'
```

（另：按进程/按名称的系统调用计数对应 syscount；off-CPU 用户栈计数对应 stackcount -u t:sched:sched_switch。）

## 13.4 单行程序示例（带输出）

对 libpthread 条件变量相关函数计数 1 秒：

```
# bpftrace -e 'u:/lib/x86_64-linux-gnu/libpthread.so.0:pthread_cond_* { @[probe] = count(); } interval:s:1 { exit(); }'
Attaching 19 probes..
@[pthread_cond_wait@@GLIBC_2.3.2]: 70
@[pthread_cond_wait]: 70
@[pthread_cond_init@@GLIBC_2.3.2]: 573
@[pthread_cond_timedwait@@GLIBC_2.3.2]: 673
@[pthread_cond_destroy@@GLIBC_2.3.2]: 939
@[pthread_cond_broadcast@@GLIBC_2.3.2]: 1796
@[pthread_cond_broadcast]: 1796
@[pthread_cond_signal]: 4600
@[pthread_cond_signal@@GLIBC_2.3.2]: 4602
```

**解读**：

- 这些 pthread 函数可能被频繁调用，所以**只跟踪 1 秒**以最小化开销
- 计数揭示了条件变量的使用方式：某些线程以 **timedwait** 定时等待监控条件变量，其他线程用 **signal**（4600 次）或 **broadcast**（1796 次）触发
- 注意每个函数常有**两个探针**（带 `@@GLIBC_2.3.2` 版本符号的和不带的），计数接近成对——分析时按功能合并
- 可继续修改单行：加入进程名称、调用栈、定时等待时长等细节

## HFT 关联

- `profile:hz:49 /comm == "策略进程名"/ { @[ustack] = count(); }` 是交易时段**最便宜**的策略热点采样（49Hz 开销 <1%，比 offcputime/pmlock 安全得多）
- `malloc sum(arg0)` 单行标注**高开销**——malloc 频率与 pmlock 同量级（每秒十万级），只能演练环境跑；交易路径的内存分配问题改用第 7 章 memleak/kmem 系列或采样法
- hardware:cache-misses 按 comm 计数：策略进程的缓存抖动（行情缓冲被逐出）可用它在共置环境快速点名

<details>
<summary>自测题</summary>

1. 为什么 pthread_cond_* 单行只跟踪 1 秒？
   <details><summary>答</summary>这些函数调用极频繁（signal 4600 次/秒），长时间跟踪 uprobe 开销叠加不可接受。</details>

2. 输出里为什么很多函数出现两行计数？
   <details><summary>答</summary>同一个函数有带 @@GLIBC_2.3.2 版本后缀和不带后缀的两个符号，uprobe 各挂一个探针，计数成对接近。</details>

3. 哪个单行被明确标注高开销、为什么？
   <details><summary>答</summary>按 ustack 统计 malloc 字节数（sum(arg0)）——malloc 调用频率极高，每次还要抓 5 层用户栈。</details>
</details>
