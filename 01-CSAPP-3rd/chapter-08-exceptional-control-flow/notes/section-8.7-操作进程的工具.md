## 8.7 操作进程的工具

> **Ch8 §8.7** · [章导读](../README.md) · 上节 [§8.6 ←](./section-8.6-非本地跳转.md) · 下节 [§8.8 →](./section-8.8-小结.md)

---

| 工具 | 用途 |
|------|------|
| `ps` | 进程列表 |
| `top`/`htop` | 实时 CPU/内存 |
| `pmap` | 地址空间映射 |
| `strace` | **跟踪 syscall** — 查意外阻塞 |
| `/proc/<pid>/` | 状态、fd、maps |

```bash
strace -c ./strategy    # syscall 统计
strace -e trace=network ./gateway
```

**HFT：** `strace` **开销巨大** — 只在测试环境查「谁在 syscall」；生产用 `perf`/`bpftrace`。

---

### 口述巩固 · 自测

1. （待口述补）本节核心一句话？

---

← [§8.6 ←](./section-8.6-非本地跳转.md) · [本章导读](../README.md) · [§8.8 →](./section-8.8-小结.md)
