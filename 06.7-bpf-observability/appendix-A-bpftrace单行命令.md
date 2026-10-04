# 附录 A bpftrace 单行程序

> 底本：《BPF之巅》附录 A（印刷 p770–774），选取书中各章用到的单行程序

```bash
# 语法骨架：bpftrace -e 'probe /filter/ { action; }'
```

## 第 6 章 CPU

```bash
# 跟踪新进程，包括进程参数
bpftrace -e 'tracepoint:syscalls:sys_enter_execve { join(args->argv); }'

# 以 99Hz 的频率采样正在运行的进程名
bpftrace -e 'profile:hz:99 { @[comm] = count(); }'

# 以 49Hz 的频率采样进程 ID 为 189 的用户态调用栈信息
bpftrace -e 'profile:hz:49 /pid == 189/ { @[ustack] = count(); }'

# 跟踪通过 pthread_create() 创建的新线程
bpftrace -e 'u:/lib/x86_64-linux-gnu/libpthread-2.27.so:pthread_create
    { printf("%s by %s (%d)\n", probe, comm, pid); }'
```

## 第 7 章 内存

```bash
# 根据用户态调用栈统计进程堆内存扩展（brk()）
bpftrace -e 'tracepoint:syscalls:sys_enter_brk { @[ustack, comm] = count(); }'

# 按进程统计缺页错误 / 按用户态调用栈统计缺页错误
bpftrace -e 'software:faults:1 { @[comm] = count(); }'
bpftrace -e 'software:faults:1 { @[ustack, comm] = count(); }'

# 通过跟踪点统计 vmscan 操作
bpftrace -e 'tracepoint:vmscan:* { @[probe]++; }'
```

## 第 8 章 文件系统

```bash
# 按进程名统计通过 open(2) 打开的文件
bpftrace -e 't:syscalls:sys_enter_open { printf("%s %s\n", comm, str(args->filename)); }'

# 展示 read() 请求大小分布
bpftrace -e 'tracepoint:syscalls:sys_enter_read { @ = hist(args->count); }'

# 展示 read() 实际读取字节数（以及错误，负值）
bpftrace -e 'tracepoint:syscalls:sys_exit_read { @ = hist(args->ret); }'

# 统计 VFS 调用
bpftrace -e 'kprobe:vfs_* { @[probe] = count(); }'

# 统计 ext4 跟踪点
bpftrace -e 't:ext4:* { @[probe] = count(); }'
```

## 第 9 章 磁盘 I/O

```bash
# 统计块 I/O 跟踪点
bpftrace -e 't:block:* { @[probe] = count(); }'

# 以直方图统计块 I/O 尺寸
bpftrace -e 't:block:block_rq_issue { @ = hist(args->bytes); }'

# 统计块 I/O 请求的用户态调用栈
bpftrace -e 't:block:block_rq_issue { @[ustack] = count(); }'

# 统计块 I/O 的类型标记（rwbs）
bpftrace -e 't:block:block_rq_issue { @[args->rwbs] = count(); }'

# 按设备和 I/O 类型跟踪块 I/O 错误
bpftrace -e 't:block:block_rq_complete /args->error/
    { printf("dev %d type %s error %d\n", args->dev, args->rwbs, args->error); }'

# 统计 SCSI opcode / 结果代码 / 驱动函数
bpftrace -e 't:scsi:scsi_dispatch_cmd_start { @[args->opcode] = count(); }'
bpftrace -e 't:scsi:scsi_dispatch_cmd_done { @[args->result] = count(); }'
bpftrace -e 'kprobe:scsi_* { @[func] = count(); }'
```

## 第 10 章 网络

```bash
# 按 PID 和进程名统计套接字 accept(2) / connect(2) 调用
bpftrace -e 't:syscalls:sys_enter_accept { @[pid, comm] = count(); }'
bpftrace -e 't:syscalls:sys_enter_connect { @[pid, comm] = count(); }'

# 按在 CPU 上运行的 PID/进程名统计套接字发送和接收的字节数
bpftrace -e 'kprobe:sock_sendmsg, kretprobe:sock_recvmsg
    { @[pid, comm, retval] = sum(retval); }'

# 统计 TCP 的发送和接收次数 / 字节数直方图
bpftrace -e 'k:tcp_sendmsg { @send = count(); } kretprobe:tcp_recvmsg { @recv = count(); }'
bpftrace -e 'k:tcp_sendmsg { @send_bytes = hist(arg2); }'
bpftrace -e 'kretprobe:tcp_recvmsg { @recv_bytes = hist(retval); }'

# 按类型与远程主机（仅 IPv4）统计 TCP 重传
bpftrace -e 't:tcp:tcp_retransmit_* { @[probe, ntop(2, args->saddr)] = count(); }'

# 以直方图统计 UDP 发送的字节数
bpftrace -e 'k:udp_sendmsg { @send_bytes = hist(arg2); }'

# 统计发送数据包时的内核态调用栈
bpftrace -e 't:inet:net_dev_xmit { @[kstack] = count(); }'
```

## 第 11 章 安全

```bash
# 为 PID 1234 的进程统计安全审计事件数
bpftrace -e '/pid == 1234/ { @[probe] = count(); }'

# 跟踪 PAM 会话开始
bpftrace -e 'u:/lib/x86_64-linux-gnu/libpam.so.0:pam_start
    { printf("%s: %s\n", str(arg0), str(arg1)); }'

# 跟踪内核模块加载
bpftrace -e 't:module:module_load { printf("load: %s\n", str(args->name)); }'
```

## 第 13 章 应用程序

```bash
# 按用户态调用栈计算 malloc() 请求字节总数（高开销！）
bpftrace -e 'u:libc:malloc { @[ustack] = sum(arg0); }'

# 跟踪 kill() 信号：发送进程名、目标 PID、信号号码
bpftrace -e 't:syscalls:sys_enter_kill
    { printf("%s PID %d signal %d\n", comm, args->pid, args->sig); }'

# 对 libpthread 互斥锁方法计数 1 秒
bpftrace -e 'u:/lib/x86_64-linux-gnu/libpthread.so.0:pthread_mutex*lock
    { @[probe] = count(); } interval:s:1 { exit(); }'

# 对 libpthread 条件变量函数计数 1 秒
bpftrace -e 'u:/lib/x86_64-linux-gnu/libpthread.so.0:pthread_cond*
    { @[probe] = count(); } interval:s:1 { exit(); }'
```

## 第 14 章 内核

```bash
# 按系统调用函数对系统调用计数
bpftrace -e 'tracepoint:raw_syscalls:sys_enter { @[ksym(*(kaddr("sys_call_table")
    + args->id * 8))] = count(); }'

# 对以 "attach" 开始的内核函数计数
bpftrace -e 'kprobe:attach* { @[probe] = count(); }'

# 为内核函数 vfs_read() 计时并总结为直方图（双探针计时模板）
bpftrace -e 'k:vfs_read { @ts[tid] = nsecs; }
    kr:vfs_read /@ts[tid]/ { @ = hist(nsecs - @ts[tid]); delete(@ts[tid]); }'

# 对内核函数 "func1" 第一个整数参数的出现频率计数
bpftrace -e 'kprobe:func1 { @[arg0] = count(); }'

# 对内核函数 "func1" 返回值的出现频率计数
bpftrace -e 'kretprobe:func1 { @[retval] = count(); }'

# 以 99Hz 采样内核态调用栈，不包含 idle
bpftrace -e 'profile:hz:99 /pid != 0/ { @[kstack] = count(); }'

# 对上下文切换调用栈计数
bpftrace -e 'tracepoint:sched:sched_switch { @[kstack] = count(); }'

# 按内核函数对工作队列请求计数
bpftrace -e 't:workqueue:workqueue_execute_start { @[ksym(args->function)] = count(); }'
```

## HFT 关联

这一页可打印贴墙：行情机排障 90% 的场景（进程创建、网络收发、TCP 重传、大 read、锁风暴）都有对应单行。注意书中的库路径是 glibc 2.27 时代的，用前先 `ldd` 确认本机 libc/libpthread 版本。

<details>
<summary>自测题</summary>

1. 双探针计时模板为什么用 `tid` 做键、kretprobe 里为什么带 `/@ts[tid]/` 过滤？
2. `sys_exit_read` 的 hist(args->ret) 为什么能看到"错误"？
3. 哪个单行被原书标注"高开销"？为什么？

</details>
