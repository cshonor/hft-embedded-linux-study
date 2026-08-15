# 4. BCC 工具检查清单（BPF 深探）

传统工具给出 **方向** 后，用 BCC **精准打击**（包名可能是 `*-bpfcc` 或短名）：

### 进程与文件

| 工具 | 用途 |
|------|------|
| **`execsnoop`** | 短生命周期进程 — 疯狂 fork 的脚本/守护进程 |
| **`opensnoop`** | 文件打开 — 配置路径、ENOENT、日志位置 |

```bash
sudo execsnoop-bpfcc
sudo opensnoop-bpfcc
```

### 文件系统与磁盘 I/O

| 工具 | 用途 |
|------|------|
| **`ext4slower`** | ext4 慢操作（其他 fs 有 `xfs*`、`btrfs*` 等变体） |
| **`biolatency`** | 块 I/O **延迟直方图** — 长尾、多峰 |
| **`biosnoop`** | 每次 I/O 一行 — 谁、多大、多慢 |
| **`cachestat`** | 页缓存 hit/miss |

```bash
sudo biolatency-bpfcc -F -m 5 10
sudo cachestat-bpfcc 5
```

### 网络

| 工具 | 用途 |
|------|------|
| **`tcpconnect`** | 主动 outbound 连接 |
| **`tcpaccept`** | 被动 inbound 连接 |
| **`tcpretrans`** | **TCP 重传** — 丢包/拥塞信号 |

```bash
sudo tcpretrans-bpfcc
sudo tcpconnect-bpfcc
```

→ HFT 延伸：[note-XDP与tc-BPF.md](../../note-XDP与tc-BPF.md) · [chapter-10-网络.md](../../chapter-10-networking/)

### CPU 调度与剖析

| 工具 | 用途 |
|------|------|
| **`runqlat`** | CPU **运行队列等待** 直方图 — 饱和度 |
| **`profile`** | ~49 Hz **栈采样** — CPU 热点火焰图原料 |

```bash
sudo runqlat-bpfcc 10
sudo profile-bpfcc -F 99 30   # 频率可调；99 避免与 tick 锁步
```

---
