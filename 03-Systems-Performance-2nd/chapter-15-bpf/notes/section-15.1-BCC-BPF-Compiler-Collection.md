## 15.1 BCC (BPF Compiler Collection)

### 是什么

| 组成 | 说明 |
|------|------|
| **bcc 库** | C/Python/Lua 写 BPF，编译加载 |
| **bcc-tools** | 大量 **预制单用途工具** — `/usr/share/bcc/tools/` |
| **libbpf 时代** | 新工具渐迁 **libbpf + CO-RE** — 04-BPF 专书详述 |

### 安装与危机清单

```bash
# Debian/Ubuntu 示例
sudo apt install bpfcc-tools linux-headers-$(uname -r)

# 验证
ls /usr/share/bcc/tools/ | head
sudo biolatency-bpfcc -h 2>/dev/null || sudo biolatency -h
```

→ Ch 4 [危机工具包](../../chapter-04-observability-tools/)

### 工具地图（与前文章节对照）

| 领域 | BCC 工具 | SysPerf 章 |
|------|----------|------------|
| **CPU** | `profile`, `runqlat`, `runqlen`, `cpudist` | Ch 6 |
| **内存** | `drsnoop`, `wss` | Ch 7 |
| **文件/盘** | `opensnoop`, `filetop`, `cachestat`, `biolatency`, `biosnoop`, `biotop`, `biostacks` | Ch 8–9 |
| **网络** | `tcplife`, `tcptop`, `tcpretrans`, `tcpconnect`, `gethostlatency` | Ch 10 |
| **进程** | `execsnoop`, `execsnoop` | Ch 5 |
| **综合** | `hardirqs`, `softirqs`, `offcputime` | Ch 5–6 |

```bash
# 调度延迟分布（Ch 6 金标准）
sudo runqlat-bpfcc 10

# 块 I/O 延迟直方图（Ch 9）
sudo biolatency-bpfcc -F -m 5 10

# TCP 连接生命周期（Ch 10）
sudo tcplife-bpfcc

# Off-CPU 栈（Ch 5 — 与 perf 互补）
sudo offcputime-bpfcc -p $(pidof strategy) 30
```

### BCC 适用场景

| 适合 | 例子 |
|------|------|
| **标准工具日常化** | runbook 固定几条 BCC |
| **复杂多事件工具** | 需状态机、多 map 协作 |
| **打包给团队** | Python CLI 封装 |

**开发：** Python + BCC — 比 bpftrace 冗长，但 **可维护、可发布**。

→ [04-BPF ch04 BCC](../../../04-BPF-Performance-Tools/chapter-04-bcc/)

---


---

← [本章导读](../README.md)
