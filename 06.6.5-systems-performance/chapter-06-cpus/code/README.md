# ch06 code · CPU 排查与调优脚本

> 本章笔记中的命令序列落盘；说明性伪代码块保留在笔记内。

## scripts/

| 脚本 | 出处 | 内容 | 验证（2026-10 Ubuntu 24.04） |
|------|------|------|------|
| [cpu-triage.sh](./scripts/cpu-triage.sh) | 6.5 块3、6.6 块4/5 | CPU 排查六连：uptime/mpstat/vmstat/pidstat -t/PSI/perf stat -a + runqlat | `bash -n` ✓；mpstat/vmstat/pidstat/PSI 段实跑 ✓ |
| [ipc-measure.sh](./scripts/ipc-measure.sh) | 6.1 块1、6.4 块2 | IPC 测量（程序级/全系统）+ 读数口诀 | `bash -n` ✓（perf 需 paranoid≤2，本机=4） |
| [cpu-load-gen.sh](./scripts/cpu-load-gen.sh) | 6.8 块6/7/8 | 负载生成三式：busy loop（可绑核）/ sysbench / stress-ng 五连 | `bash -n` ✓；sysbench、stress-ng 本机**未装**（`sudo apt install sysbench stress-ng`） |
| [cpu-tuning-verify.sh](./scripts/cpu-tuning-verify.sh) | 6.9 块9/10 | 调优验证：isolcpus 进程检查 / nohz 中断 / runqlat / 定频 / RT 带宽 | `bash -n` ✓；④ 实跑：本机 powersave 未调（与预期一致） |

## 运行前提

- 本机已装：mpstat、vmstat、pidstat（sysstat）、runqlat-bpfcc、perf
- 本机未装：`sysbench`、`stress-ng`（仅 cpu-load-gen.sh 的两个子命令需要）
- runqlat/perf 段需 root 或调整 `perf_event_paranoid`
