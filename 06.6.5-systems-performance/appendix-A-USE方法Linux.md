# 附录 A USE方法Linux · USE Method: Linux

> **Systems Performance 2nd** · Brendan Gregg · **精读**

> **定位：** 附录 A 把 USE 方法（Utilization / Saturation / Errors）落到 Linux 具体命令——前面各章反复引用的「附录 A」就是这里。

## USE 方法速查表（Linux 落地）

### CPU

| 字母 | 指标 | 命令 |
|------|------|------|
| **U** | 每 CPU 利用率 | `mpstat -P ALL 1` |
| **S** | run queue 长度 / 调度延迟 | `vmstat 1`（`r` 列）、`runqlat`（BCC）、PSI `/proc/pressure/cpu` |
| **E** | 硬件错误 | `mcelog`、EDAC、`dmesg | grep -i mce` |

### 内存

| 字母 | 指标 | 命令 |
|------|------|------|
| **U** | 物理/虚拟内存使用 | `free -h`、`/proc/meminfo`、`sar -r` |
| **S** | swap / direct reclaim / OOM | `vmstat 1`（`si`/`so`）、`sar -B`、PSI `/proc/pressure/memory` |
| **E** | 分配失败 / ECC | `dmesg | grep -i oom`、EDAC |

### 网络接口

| 字母 | 指标 | 命令 |
|------|------|------|
| **U** | 吞吐 / 协商带宽 | `sar -n DEV`、`nicstat`、`ip -s link` |
| **S** | 队列满 / 重传 / backlog | `ss -lnt`（Send-Q/Recv-Q）、`netstat -s` retrans |
| **E** | CRC / frame / drop | `ethtool -S`、`ip -s link` errors |

### 存储设备

| 字母 | 指标 | 命令 |
|------|------|------|
| **U** | %util | `iostat -x 1`、`sar -d` |
| **S** | 队列深度 / await | `iostat -x 1`（`avgqu-sz`、`await`） |
| **E** | 错误计数 | `smartctl -a`、`/proc/diskstats` |

## HFT 60 秒清单（USE 视角）

```bash
# CPU
mpstat -P ALL 1 5          # 每核利用率 + sys%
vmstat 1 5                  # r 列（饱和度）+ si/so（内存）

# 内存
free -h                     # MemAvailable
cat /proc/pressure/memory   # PSI stall

# 网络
ss -s                       # 连接总数
ss -tiepm | head -20        # RTT + retrans + queue
ethtool -S eth0 | grep -i drop

# 磁盘
iostat -x 1 5               # %util + avgqu-sz + await
```

### 常见陷阱

1. **只查 Utilization 不查 Saturation**——HFT 延迟尖刺多因饱和度（run queue/backlog/队列深度）而非利用率；30% 利用率但 run queue 满 = 瓶颈
2. **USE 只跑一遍**——应在基线/异常/调优后各跑一次，对比才能判断什么是「异常」
3. **容器内跑 USE 不看 cgroup**——容器看到的 /proc 是 cgroup 限额后的，需要同时看宿主机级和 cgroup 级

<details>
<summary>自测题（点击展开）</summary>

1. USE 方法的三个字母分别代表什么？
   <details><summary>答</summary>Utilization（利用率）、Saturation（饱和度）、Errors（错误）——每个资源都要查这三项</details>
2. HFT 60 秒清单中 CPU 饱和度看什么？
   <details><summary>答</summary>vmstat 的 `r` 列（run queue 长度）和 PSI `/proc/pressure/cpu`——r 持续 > 0 说明调度饱和</details>
3. 为什么容器内跑 USE 不够？
   <details><summary>答</summary>容器 /proc 是 cgroup 限额后的视图——看到的 CPU/内存和宿主机不同，需同时看 cgroup metrics</details>

</details>

## 相关章节

- 上一章：[chapter-16-case-studies](./chapter-16-case-studies/)
- 下一章：[appendix-B-sar总结.md](./appendix-B-sar总结.md)
