# ch01 code · 命令脚本

> 本章笔记中的**可执行命令序列**落盘为脚本；说明性伪代码块（分析框架/决策树）保留在笔记内，不落盘。

## scripts/

| 脚本 | 出处 | 内容 | 验证 |
|------|------|------|------|
| [hft-10s-triage.sh](./scripts/hft-10s-triage.sh) | 1.9 块9 | HFT 10 秒三连：IRQ 分布 → mpstat → 亲和性 → perf top | `bash -n` ✓（2026-10 Ubuntu 24.04；mpstat/ethtool/perf 本机可用） |
| [layers-observability.sh](./scripts/layers-observability.sh) | 1.7 块4 | 四层递进：计数器 → 指标 → 剖析 → 追踪 | `bash -n` ✓；perf 段需 `perf_event_paranoid≤2`（本机=4，需 sudo 调整或 root 跑） |
| [microbench-workflow.sh](./scripts/microbench-workflow.sh) | 1.8 块7 | 微观基准流程：isolcpus 检查 → taskset 绑核 → 程序内 rdtsc 规范 | `bash -n` ✓ |

## 运行前提

- `mpstat`（sysstat 包）、`ethtool`、`numastat`、`perf`、`taskset`：本机均已确认安装
- perf 相关段：本机 `kernel.perf_event_paranoid=4`，非 root 需先
  `sudo sysctl kernel.perf_event_paranoid=2`（或更低）
- 火焰图两步（`stackcollapse-perf.pl` / `flamegraph.pl`）需 FlameGraph 仓库，本机未装，
  用到时 `git clone https://github.com/brendangregg/FlameGraph`
