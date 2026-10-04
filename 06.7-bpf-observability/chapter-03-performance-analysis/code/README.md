# 02-ch03 code · 性能分析清单

> 《BPF Performance Tools》第 3 章两个经典清单落盘。

## scripts/

| 脚本 | 出处 | 内容 | 验证（2026-10 Ubuntu 24.04） |
|------|------|------|------|
| [linux-60s.sh](./scripts/linux-60s.sh) | §3 块1 | Linux 60 秒分析十条：uptime/dmesg/vmstat/mpstat/pidstat/iostat/free/sar×2/top | `bash -n` ✓；除 dmesg 需权限外全部实跑 ✓ |
| [bcc-checklist.sh](./scripts/bcc-checklist.sh) | §4 块2 | BCC 工具清单按问题域分组（进程/FS/网络/CPU）+ 与 60s 清单的衔接用法 | `bash -n` ✓；11 个 `-bpfcc` 命令本机全部确认已装（需 root 执行） |

## 用法衔接

```
linux-60s.sh（定位资源域） → bcc-checklist.sh（BPF 下钻） → 06.6 各章专项脚本（调优）
```
