# 5. 进程与线程生命周期

### `execsnoop`

追踪 **新进程 exec** — 系统范围。

```bash
sudo execsnoop-bpfcc
```

| 场景 | 价值 |
|------|------|
| 短命 shell 循环、健康检查脚本 | CPU 被吃掉但 `top` 里一闪而过 |
| 异常 fork 风暴 | 看谁在不断拉起子进程 |

### `exitsnoop`

追踪 **进程退出**，含 **存活时长 (Age)**、退出码/信号。

```bash
sudo exitsnoop-bpfcc
```

**HFT：** 排查 watchdog 反复重启、子进程崩溃循环。

---
