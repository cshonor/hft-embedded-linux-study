# ch14 code · ftrace 脚本集

> 本章笔记中的命令序列落盘；说明性伪代码块保留在笔记内。
> **ftrace 全部操作需要 root**（写 `/sys/kernel/tracing`）。

## scripts/

| 脚本 | 出处 | 内容 | 验证（2026-10 Ubuntu 24.04） |
|------|------|------|------|
| [ftrace-101.sh](./scripts/ftrace-101.sh) | 14.1-14.4 块5-9 | tracefs 六步流程 + function tracer + filter 语法 + function_graph + 清理 | `bash -n` ✓；tracefs 已挂载确认 |
| [ftrace-events.sh](./scripts/ftrace-events.sh) | 14.5-14.10 块10-15 | tracepoint enable / filter / trigger / hist + kprobe / kretprobe / uprobe | `bash -n` ✓ |
| [ftrace-wakeup-latency.sh](./scripts/ftrace-wakeup-latency.sh) | 14.5-14.10 块16 | 合成事件测调度延迟直方图（wakeup→switch 求差），用完自动清理 | `bash -n` ✓（实跑需 root） |
| [hwlat-detect.sh](./scripts/hwlat-detect.sh) | 14.9 块17 | 硬件延迟检测（SMI/固件停顿）：60 秒窗口 + 最大停顿读数 + 清理 | `bash -n` ✓（实跑需 root） |
| [trace-cmd-cheatsheet.sh](./scripts/trace-cmd-cheatsheet.sh) | 14.11-14.13 块1-3 | trace-cmd record/report/归档/崩溃提取 + perf ftrace + 选型建议 | `bash -n` ✓；trace-cmd 已装 |

## 运行前提

- tracefs：本机已挂载于 `/sys/kernel/tracing` ✓
- **全部需要 root**（写 tracefs / 加载 kprobe）
- hwlat 的 `per-cpu` mode 需内核 ≥6.6（本机 7.0 ✓，脚本对旧内核静默跳过调参）
