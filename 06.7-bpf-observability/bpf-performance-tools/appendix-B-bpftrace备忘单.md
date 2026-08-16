# 附录 B bpftrace 备忘单

> 底本：《BPF之巅》附录 B（印刷 p776–777）

```
bpftrace -e 'probe /filter/ { action; }'
```

## 探针

| 探针 | 含义 |
|---|---|
| `BEGIN`, `END` | 程序开始和结束 |
| `tracepoint:syscalls:sys_enter_execve` | execve(2) 系统调用 |
| `tracepoint:syscalls:sys_enter_open` | open(2) 系统调用（也可跟踪 openat(2)） |
| `tracepoint:syscalls:sys_exit_read` | 跟踪 read(2) 的返回（变体） |
| `tracepoint:raw_syscalls:sys_enter` | 所有系统调用 |
| `block:block_rq_insert` | 队列块 I/O 请求 |
| `block:block_rq_issue` | 向存储设备发出块 I/O |
| `block:block_rq_complete` | 块 I/O 的完成 |
| `sock:inet_sock_set_state` | 套接字状态改变 |
| `sched:sched_process_exec` | 进程执行 |
| `sched:sched_switch` | 上下文切换 |
| `sched:sched_wakeup` | 线程唤醒事件 |
| `software:faults:1` | 缺页错误 |
| `hardware:cache-misses:1000000` | 百万分之一的 LLC 缓存未命中 |
| `kprobe:vfs_read` | 跟踪内核函数 vfs_read() |
| `kretprobe:vfs_read` | 跟踪内核函数 vfs_read() 的返回 |
| `uprobe:/bin/bash:readline` | 从 /bin/bash 跟踪 readline() |
| `uretprobe:/bin/bash:readline` | readline() 的返回 |
| `usdt:path:probe` | 从指定路径跟踪 USDT 探针 |
| `profile:hz:99` | 以 99Hz 在所有 CPU 上采样 |
| `interval:s:1` | 在一个 CPU 上每秒运行一次 |

**探针别名**：kprobe/k、kretprobe/kr、tracepoint/t、usdt、profile、hardware、software、uprobe/u、uretprobe/ur、interval。

## 变量（内置）

| 变量 | 含义 |
|---|---|
| `comm` | On-CPU 进程名 |
| `username` | 用户名字符串 |
| `tid` | On-CPU PID，**线程 ID** |
| `pid` | 进程 ID（tgid） |
| `uid` | 用户 ID |
| `kstack` / `ustack` | 内核 / 用户调用栈 |
| `nsecs` | 时间，纳秒 |
| `elapsed` | 从进程开始算起的时间，纳秒 |
| `cpu` | CPUID |
| `probe` | 当前探针全名 |
| `func` | 当前函数全名 |
| `curtask` | 指向当前 task 结构体的指针 |
| `cgroup` | 当前 cgroup ID |
| `arg0..N` | [uk]probe 参数 |
| `args->` | 跟踪点参数 |
| `retval` | [uk]retprobe 返回值 |
| `$1..$N` | CLI 参数，整数类型 |
| `str($1)..` | CLI 参数，字符串类型 |

## 动作（同步）

| 动作 | 含义 |
|---|---|
| `@map[key] = count()` | 统计频率 |
| `@map[key,...] = sum(var)` | 对变量求和 |
| `@map[key,...] = hist(var)` | 以 2 为幂的直方图 |
| `@map[key,...] = lhist(var, min, max, step)` | 线性直方图 |
| `@map[key,...] = stats(var)` | 统计：个数、均值和总数 |
| `min(var)`, `max(var)`, `avg(var)` | 最小/最大/平均值 |
| `printf("format", var0..varN)` | 打印变量（聚合用 print()） |
| `kstack(num)`, `ustack(num)` | 打印内核/用户堆栈（限定行数） |
| `ksym(ip)`, `usym(ip)` | 指令指针的内核/用户符号字符串 |
| `kaddr("name")`, `uaddr("name")` | 符号名称的内核/用户态地址 |
| `str(str[, len])` | 来自地址的字符串 |
| `ntop([af], addr)` | IP 地址到字符串 |

## 动作（异步）

| 动作 | 含义 |
|---|---|
| `printf("format", var0..varN)` | 打印变量（聚合用 print()） |
| `system("format", var0..varN)` | 运行一个命令行命令 |
| `time("format")` | 打印格式化过的时间 |
| `clear(@map)` | 清空映射表：删除所有键 |
| `print(@map)` | 打印映射表 |
| `exit()` | 退出 |

## 开关

| 开关 | 含义 |
|---|---|
| `-e 'program'` | 跟踪这个探针描述 |
| `-l 'search'` | 打印探针，而不是跟踪 |
| `-p PID` | PID |
| `-usdt-pid PID`（`-p` 对 USDT） | 对 PID 启用 USDT 探针 |
| `-c 'command'` | 运行这个命令 |
| `-v` | 详细和调试输出模式 |

## HFT 关联

与第 5 章 notes（表 5-2/5-5/5-6/5-7）互补：那一章讲原理，本页是可贴墙速查。写排障脚本时先对照"动作"表选**内核态聚合**（count/hist/sum）而不是逐事件 printf——开销差 4 倍以上（表 18-2）。

<details>
<summary>自测题</summary>

1. `t:`、`k:`、`kr:`、`u:`、`ur:` 各是什么别名？
2. tid 与 pid 的区别？双探针计时为什么用 tid 做键？
3. 同步 printf 与异步 printf 的区别？

</details>
