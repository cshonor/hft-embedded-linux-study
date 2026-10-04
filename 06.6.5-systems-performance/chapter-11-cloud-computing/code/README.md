# ch11 code · 云计算/容器排查脚本

> 本章笔记中的命令序列落盘；说明性伪代码块保留在笔记内。

## scripts/

| 脚本 | 出处 | 内容 | 验证（2026-10 Ubuntu 24.04） |
|------|------|------|------|
| [cgroup-throttle-check.sh](./scripts/cgroup-throttle-check.sh) | 11.3 块1/2 | CPU 节流三件套：cpu.max / cpu.stat(nr_throttled) / watch 模式盯涨势 | `bash -n` ✓；cpu.stat 段实跑 ✓（本机无节流） |

## 运行前提

- cgroup v2（本机已是）；容器内跑看自己的 `/sys/fs/cgroup/`
- 宿主机按 cgroup 追踪需 root + bpftrace
