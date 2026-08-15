# 10. BPF / bpftrace One-Liners（示意）

```bash
# 按 comm 统计 page fault（bpftrace，字段因内核而异）
bpftrace -e 'software:page-faults:1000 { @[comm] = count(); }'

# malloc 探针（短跑验证；生产用 memleak-bpfcc）
# bpftrace -e 'uprobe:/lib/x86_64-linux-gnu/libc.so.6:malloc { @bytes = sum(arg0); }'
```

→ 语法：[Ch 5](../../chapter-05-bpftrace/) · 单行集：[附录 A](../../appendix-A-bpftrace单行命令.md)

---
