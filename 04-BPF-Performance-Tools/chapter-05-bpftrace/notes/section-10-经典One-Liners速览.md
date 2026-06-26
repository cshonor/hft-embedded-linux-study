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

---
