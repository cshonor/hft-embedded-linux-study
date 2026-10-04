# ch13 code · perf 工具箱

> 本章笔记中的程序类代码块补全/落盘；说明性伪代码块保留在笔记内。

## 程序

| 文件 | 出处 | 内容 | 验证（2026-10 Ubuntu 24.04） |
|------|------|------|------|
| [false_sharing_demo.c](./false_sharing_demo.c) | 13.12 块11 | 伪共享 vs cache line 填充隔离实测（两线程各写各的计数器，打印地址证明同/异 line） | gcc 13.3 编译 ✓ 运行 ✓（packed/padded 耗时对比 + 放大倍数） |

## scripts/

| 脚本 | 出处 | 内容 | 验证 |
|------|------|------|------|
| [perf-cheatsheet.sh](./scripts/perf-cheatsheet.sh) | 13.1 块5 | 速查：健康计数/CPU 热点/火焰图/top/trace/事件勘探 | `bash -n` ✓ |
| [perf-report-flamegraph.sh](./scripts/perf-report-flamegraph.sh) | 13.10 块1/2/3 | report 三维视角 + script 定制字段 + 火焰图全管道 | `bash -n` ✓ |
| [perf-trace-summary.sh](./scripts/perf-trace-summary.sh) | 13.11 块6/7 | perf trace 汇总模式 + mmap/munmap→TLB shootdown 尖刺案例套路 | `bash -n` ✓ |
| [perf-advanced.sh](./scripts/perf-advanced.sh) | 13.12 块8-13、13.3 块14 | 进阶五式：sched/lock/c2c/mem/annotate + uprobe | `bash -n` ✓ |
| [perf-stat-ab.sh](./scripts/perf-stat-ab.sh) | 13.8 块15 | stat 三用法：IPC 趋势 / `-u` 只看用户态 / `-r 5` A/B 实验纪律 | `bash -n` ✓ |
| [perf-full-workflow.sh](./scripts/perf-full-workflow.sh) | 13.10 块4 | 计数→on-CPU→off-CPU 三段式（顺序不能乱） | `bash -n` ✓ |

## 运行前提

- perf 全部命令：本机 `perf_event_paranoid=4`，非 root 需 `sudo sysctl kernel.perf_event_paranoid=2`
  （record/c2c/mem/lock 等对硬件 PMU 还有额外要求，虚拟机里可能无 PMU）
- 火焰图：`git clone https://github.com/brendangregg/FlameGraph` 并加入 PATH
- off-CPU 段（offcputime-bpfcc）需 root
