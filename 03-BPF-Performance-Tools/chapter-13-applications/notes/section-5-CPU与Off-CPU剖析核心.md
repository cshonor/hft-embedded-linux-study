# 5. CPU 与 Off-CPU 剖析（核心）

### `profile` — On-CPU 火焰图

```bash
sudo profile-bpfcc -F 99 -p $(pidof my_strategy) 30
```

找出 **最耗 CPU 的应用代码路径** — 需 [Ch 12 帧指针 + 符号](../../chapter-12-languages/)。

→ [Ch 6 § profile](../../chapter-06-cpus/)

### `offcputime` / `offcpuhist` — Off-CPU 火焰图 🔴

**应用延迟神器** — 线程 **为何离开 CPU**（锁、I/O、sleep）。

```bash
sudo offcputime-bpfcc -p $(pidof my_strategy) 30
sudo offcpuhist-bpfcc -p $(pidof myapp) 10   # 直方图形态
```

| Off-CPU 栈顶 | 常见原因 |
|--------------|----------|
| `futex` / `pthread_mutex_*` | **锁竞争** |
| `read` / `write` / `recv` | I/O / 网络 |
| `nanosleep` | 人为 sleep |
| `epoll_wait` | 正常等事件（看 wait 时长） |

**HFT runbook：** P99 升 + CPU 不忙 → **先 `offcputime`**，再决定查网/盘/锁。

---
