# 7. CPU 使用时间与剖析 (On-CPU)

### `cpudist`

统计线程每次 **被调度上 CPU 后连续运行多久** 的分布（时间片长度分布）。

```bash
sudo cpudist-bpfcc -p $(pidof myapp) 10
```

与 `runqlat` 互补：一个看 **等 CPU 多久**，一个看 **上 CPU 后跑多久**。

### `cpufreq`

采样 CPU **实际运行频率** — 是否因省电策略降频。

```bash
sudo cpufreq-bpfcc 5
```

**HFT 生产：** 交易机通常 **performance governor** + 关 C-states；若频率掉下去，延迟会莫名变差。

### `profile` — CPU 栈采样 🔴

按固定频率（如 **99Hz**）采样 **全栈**，统计次数 — 生成火焰图的输入。

```bash
sudo profile-bpfcc -F 99 30
sudo profile-bpfcc -F 99 -p $(pidof myapp) 30
```

| 参数 | 说明 |
|------|------|
| `-F` | 采样频率 Hz |
| `-p` | 仅某进程 |
| `-U` | 仅用户态栈 |

**与 `perf record`：** 同属 on-CPU 采样；BCC 版便于与书中其他 BCC 工具一致、脚本化。

### `syscount`（关联）

按 **系统调用类型** 计数 — 回答「CPU 时间是否耗在 syscall 上」。

```bash
sudo syscount-bpfcc -i 1
```

---
