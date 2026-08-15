# 9. BPF / bpftrace One-Liners（示意）

```bash
# 块 I/O 延迟直方图（优先用 biolatency-bpfcc）
# bpftrace -e 'tracepoint:block:block_rq_complete { @us = hist(((args->sector) * 512)); }'

# 按 comm 计数 block 完成事件（粗筛）
bpftrace -e 'tracepoint:block:block_rq_complete { @[comm] = count(); }'
```

→ 生产固定用 BCC 工具；bpftrace 验证 tracepoint 名与内核版本。

---
