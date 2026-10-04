# ch09 code · 磁盘排查与观测脚本

> 本章笔记中的命令序列落盘；说明性伪代码块保留在笔记内。

## scripts/

| 脚本 | 出处 | 内容 | 验证（2026-10 Ubuntu 24.04） |
|------|------|------|------|
| [disk-triage.sh](./scripts/disk-triage.sh) | 9.5 块3 | 排查五连：iostat -xz/pidstat -d/PSI io/dmesg 存储错误/swap 活动 | `bash -n` ✓；iostat/pidstat 段实跑 ✓ |
| [disk-queue-inspect.sh](./scripts/disk-queue-inspect.sh) | 9.4 块1/2 | 调度器 + 队列参数解读（nr_requests/read_ahead_kb/rq_affinity/io_poll…） | `bash -n` ✓；本机 nvme0n1 实跑 ✓ |
| [disk-observability.sh](./scripts/disk-observability.sh) | 9.6 块4/5 | biolatency/biosnoop/biostacks + 2 条 bpftrace（I/O 大小直方图、>10ms 慢 I/O） | `bash -n` ✓；biolatency/biosnoop-bpfcc 已装；biostacks-bpfcc **未装**（脚本有替代提示） |
| [disk-bench-tuning.sh](./scripts/disk-bench-tuning.sh) | 9.7 块6/7 | fio 4k 随机读 QD1 延迟验收（--readonly 安全）/ ionice idle 类 / cgroup io.max | `bash -n` ✓；fio、ioping **未装** |

## 运行前提

- 已装：iostat、pidstat（sysstat）、ionice、biolatency/biosnoop-bpfcc
- 未装：`fio`、`ioping`、`biostacks-bpfcc`
- BPF 工具段全部需 root
