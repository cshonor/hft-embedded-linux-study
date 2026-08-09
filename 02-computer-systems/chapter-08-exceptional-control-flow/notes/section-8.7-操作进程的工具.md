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

### 自测题

<details>
<summary>1. Linux 中查看和管理进程的常用工具有哪些？</summary>

`ps`（快照）、`top`/`htop`（实时）、`pgrep`/`pkill`（按名查找/杀）、`strace`（追踪系统调用）、`lsof`（查看打开的文件/fd）、`/proc/<pid>/`（进程信息伪文件系统，可看 maps/status/fd 等）。HFT 调试常用 `perf stat` 看硬件计数器、`/proc/<pid>/status` 看内存和切换次数。

</details>

<details>
<summary>2. /proc/<pid>/maps 能看到什么？HFT 调试中有什么用？</summary>

显示进程的虚拟地址空间映射——每段的地址范围、权限(rwxp)、映射文件(如 .so)、offset。HFT 调试中用于：确认 hugepage 是否生效、检查是否有意外的内存映射、排查段错误地址落在哪个映射区。`/proc/<pid>/smaps` 还提供每段的 RSS/PSS 等详细内存统计。

</details>


---

← [§8.6 ←](./section-8.6-非本地跳转.md) · [本章导读](../README.md) · [§8.8 →](./section-8.8-小结.md)
