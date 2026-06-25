# 3. Linux 60 秒检查清单（传统工具）

登录 **性能异常机器后前 60 秒** — **先跑这些，再开 BPF**。BPF 书仍强调传统工具 **粗筛方向**。

| # | 命令 | 看什么 |
|---|------|--------|
| 1 | `uptime` | load average — 整体资源压力 |
| 2 | `dmesg \| tail` | OOM、TCP 丢包、驱动错误 |
| 3 | `vmstat 1` | run queue、swap、user/sys CPU |
| 4 | `mpstat -P ALL 1` | **单核打满** vs 多核均衡 |
| 5 | `pidstat 1` | 哪个进程吃 CPU（随时间滚动） |
| 6 | `iostat -xz 1` | 磁盘 **`await`**（响应时间）、**`%util`** |
| 7 | `free -m` | **available**、buff/cache |
| 8 | `sar -n DEV 1` | 网卡吞吐是否顶满 |
| 9 | `sar -n TCP,ETCP 1` | TCP 连接率、**重传** |
| 10 | `top` | 综合核对 |

```bash
# 一键习惯：先 1–3，再按疑点加 4–9
uptime
dmesg | tail
vmstat 1
```

> **HFT 共置裸机：** 块设备 `iostat` 常 ⚪（无本地盘）；**8–9 网络** 与 **4–5 CPU** 通常是 60 秒重点。与 [SysPerf Ch 4 危机工具](../../../02-Systems-Performance-2nd/chapter-04-observability-tools/) 对照。

---
