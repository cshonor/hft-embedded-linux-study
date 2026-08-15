# 7. BPF 单行命令 (One-Liners)

安全场景 **ad hoc** 极有价值 — PAM/SSH/sudo 监控示例（概念）：

```bash
# 监控 PAM 会话启动（tracepoint 名因发行版/版本而异，先用 bpftrace -l 搜索 pam）
bpftrace -e 'tracepoint:pam:* { printf("%s %s\n", comm, probe); }'

# 追踪 sudo 执行（示意）
bpftrace -e 'tracepoint:syscalls:sys_enter_execve /comm == "sudo"/ {
    printf("sudo by %s\n", comm);
}'
```

**原则：** 应急响应 **先 `-l` 列 probe** → 短跑验证 → 固化 BCC 脚本或 SIEM 规则。

→ [Ch 5](../../chapter-05-bpftrace/) · [附录 A](../../appendix-A-bpftrace单行命令.md)

---
