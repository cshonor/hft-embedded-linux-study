# 3. 传统文件系统工具

| 工具 | 用途 | 注意 |
|------|------|------|
| `df -h` | 文件系统容量 | |
| `mount` | 挂载选项（`noatime`、`barrier`…） | |
| `strace -e open,read,write` | 系统调用级追踪 | **开销极高** — 仅短跑 |
| `perf trace` | 轻量 syscall 采样 | 比 strace 克制 |
| `fatrace` | fanotify 文件访问 | 专用、有场景 |

```bash
df -h
mount | grep -E 'ext4|xfs'
```

**BPF 价值：** 比 `strace` 更适合 **生产短窗口** — 内核聚合、可限频率。

---
