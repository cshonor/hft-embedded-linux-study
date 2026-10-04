# ch16 code · 案例演练脚本

> 本章笔记中的命令序列落盘；说明性伪代码块保留在笔记内。

## scripts/

| 脚本 | 出处 | 内容 | 验证（2026-10 Ubuntu 24.04） |
|------|------|------|------|
| [config-snapshot.sh](./scripts/config-snapshot.sh) | 16.1.3 块2 | 配置快照（部署钩子调用）：sysctl/cmdline/mounts/interrupts/lscpu/cpufreq/dmesg/包清单，版本化目录 + diff 用法 | `sh -n` ✓ 实跑 ✓（/tmp 落地，9 个文件生成） |
| [unexplained-win-drill.sh](./scripts/unexplained-win-drill.sh) | 16.1 块1-4、16.9 块5-9 | "莫名变快"演练全流程：S1 分母核查 → S2 sar 同期对比 → S3 配置事实（THP/亲和/governor）→ S4 九事件 stat → S5 on/off-CPU 分野 | `bash -n` ✓；S2/S3 段实跑 ✓ |

## 运行前提

- sar 历史对比需 sysstat 采集开启；perf/BPF 段需权限调整或 root
- config-snapshot.sh 默认尝试 `/var/lib/perf-snap`（无权限自动落 /tmp）
