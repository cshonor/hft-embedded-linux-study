# ch04 code · 观测工具脚本

> 本章笔记中的命令序列落盘；说明性伪代码块保留在笔记内。

## scripts/

| 脚本 | 出处 | 内容 | 验证（2026-10 Ubuntu 24.04） |
|------|------|------|------|
| [perf-install-check.sh](./scripts/perf-install-check.sh) | 4.1 块1/2/3 | perf 正确安装（`linux-tools-$(uname -r)`）与出数验证 | `bash -n` ✓；验证段实跑 ✓（本机 paranoid=4，硬件计数需 sudo 调低） |
| [sar-cheatsheet.sh](./scripts/sar-cheatsheet.sh) | 4.4 块4/5 | sar 实时 7 项采样 + `/var/log/sysstat` 历史回放 | `bash -n` ✓；历史回放需 sysstat 开启采集 |
| [low-overhead-tracing.sh](./scripts/low-overhead-tracing.sh) | 4.5 块6 | 生产低开销追踪：`perf trace` / bpftrace 聚合替代 `strace -p` | `bash -n` ✓（bpftrace 段需 root） |
| [observer-effect-bench.sh](./scripts/observer-effect-bench.sh) | 4.6 块7 | 观测开销自测框架：基线 vs 挂观测，差值=观测对被测指标的影响 | `bash -n` ✓（框架脚本，需配合自己的 benchmark） |

## 运行前提

- `sar`/`mpstat`：sysstat 包（本机已装）；历史回放需 `/etc/default/sysstat` 开 `ENABLED="true"`
- `perf trace` / `perf record`：`perf_event_paranoid≤2` 或 root
- bpftrace：本机 v0.20.2 已装，执行需 root
