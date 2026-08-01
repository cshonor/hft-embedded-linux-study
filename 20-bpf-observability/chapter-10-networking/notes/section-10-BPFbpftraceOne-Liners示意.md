# 10. BPF / bpftrace One-Liners（示意）

```bash
# TCP 重传（生产用 tcpretrans-bpfcc）
# bpftrace -e 'kprobe:tcp_retransmit_skb { printf(...); }'

# 按 comm 统计 connect
bpftrace -e 'tracepoint:syscalls:sys_enter_connect { @[comm] = count(); }'

# 采样内核网络栈（短跑）
bpftrace -e 'kprobe:tcp_sendmsg { @[kstack] = count(); }'
```

→ [Ch 5 bpftrace](../../chapter-05-bpftrace/) · [附录 A](../../appendix-A-bpftrace单行命令.md)

---
