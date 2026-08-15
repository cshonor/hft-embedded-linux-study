# 10. 经典 One-Liners 速览

以下可在 **数秒** 内验证假设；完整清单见 [附录 A](../../appendix-A-bpftrace单行命令.md) 与 [附录 B 备忘单](../../appendix-B-bpftrace备忘单.md)。

```bash
# 谁在读盘（按进程计数）
bpftrace -e 'tracepoint:syscalls:sys_enter_read { @[comm] = count(); }'

# 新进程
bpftrace -e 'tracepoint:syscalls:sys_enter_execve { printf("%s %s\n", comm, str(args->filename)); }'

# 每 CPU 采样栈（CPU 热点）
bpftrace -e 'profile:hz:99 { @[kstack] = count(); }'

# TCP 连接（示意，字段随内核版本调整）
bpftrace -e 'kprobe:tcp_connect { printf("connect pid=%d\n", pid); }'

# 某 PID 的 open 路径
bpftrace -e 'tracepoint:syscalls:sys_enter_openat /pid == 1234/ {
    printf("%s\n", str(args->filename));
}'
```

**HFT 用法：** incident 窗口内 **短跑 30–60s** → 确认嫌疑 → 再换 BCC 工具（`runqlat`、`profile-bpfcc`）长一点采集。


### 常见陷阱

1. **直接复制 one-liner 不修改参数** — one-liner 中的函数名、PID 是示例值，需替换为实际目标；直接运行可能因目标不存在而无输出
2. **忽视 one-liner 的开销评估** — 某些 one-liner（如全系统 syscall 追踪）开销很大；在生产环境运行前应评估事件频率和 probe 开销
3. **只记 one-liner 不理解原理** — one-liner 是速查工具，理解 probe 类型、Map 操作、聚合函数的原理才能灵活变通解决新问题

<details>
<summary>📝 自测题（点击展开）</summary>

1. **5 个最常用的 bpftrace one-liner 是什么？**

   <details>
   <summary>参考答案</summary>

   (1) 统计函数调用次数：`kprobe:do_sys_open { @++ }`；(2) 按进程统计 syscall：`tracepoint:raw_syscalls:sys_enter { @[comm] = count() }`；(3) 函数延迟直方图：`kprobe:vfs_read { @start[tid]=nsecs } kretprobe:vfs_read /@start[tid]/ { @lat=hist(nsecs-@start[tid]) }`；(4) CPU 热点采样：`profile:hz:99 { @[kstack] = count() }`；(5) 新进程追踪：`tracepoint:sched:sched_process_exec { printf("%s\n", comm) }`。

   </details>

2. **如何把 one-liner 改编为自己的追踪脚本？**

   <details>
   <summary>参考答案</summary>

   (1) 替换 probe 目标——把 `do_sys_open` 改成你关心的函数；(2) 添加 filter——`/pid == 1234/` 只看特定进程；(3) 修改聚合方式——`count()` → `sum(arg2)` → `hist(nsecs-@start[tid])`；(4) 自定义输出——加 `BEGIN` 打印表头，`END` 用 `print()` 格式化。

   </details>

3. **HFT 排障中最有用的 bpftrace one-liner 是什么？**

   <details>
   <summary>参考答案</summary>

   延迟分布直方图：`kprobe:tcp_sendmsg { @s[tid]=nsecs } kretprobe:tcp_sendmsg /@s[tid]/ { @lat=hist(nsecs-@s[tid]); delete(@s[tid]) }`——一行命令看到 sendmsg 延迟的完整分布，立即判断是否有微秒级异常。配合 `/comm == "myapp"/` 过滤只看目标进程。

   </details>

</details>

---
