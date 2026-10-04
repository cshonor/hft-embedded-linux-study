# ch05 code · 应用观测脚本

> 本章笔记中的命令序列落盘；说明性伪代码块保留在笔记内。

## scripts/

| 脚本 | 出处 | 内容 | 验证（2026-10 Ubuntu 24.04） |
|------|------|------|------|
| [app-observability.sh](./scripts/app-observability.sh) | 5.5 块1/2/3 | 应用观测四件套：`syscount-bpfcc` / `perf record -F 99`（或 `profile-bpfcc`）/ `offcputime-bpfcc` / `pidstat -t` | `bash -n` ✓；四个命令本机均已装（bpfcc-tools 0.29.1、sysstat） |

## 运行前提

- `*-bpfcc` 命令需 root（BPF 程序加载）
- `perf record` 需 `perf_event_paranoid≤2` 或 root（本机=4）
- 目标进程替换 `$PROC`；`pidof` 取多实例中的第一个
