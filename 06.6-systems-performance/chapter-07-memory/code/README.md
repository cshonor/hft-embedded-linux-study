# ch07 code · 内存排查与调优脚本

> 本章笔记中的命令序列落盘；说明性伪代码块保留在笔记内。

## scripts/

| 脚本 | 出处 | 内容 | 验证（2026-10 Ubuntu 24.04） |
|------|------|------|------|
| [mem-triage.sh](./scripts/mem-triage.sh) | 7.1 块1、7.4 块3/4 | 内存排查六连：free(available)/vmstat(si/so)/PSI/slabtop/OOM 历史/numastat | `bash -n` ✓；free/vmstat/PSI 段实跑 ✓ |
| [numa-check.sh](./scripts/numa-check.sh) | 7.3 块2、7.6 块7 | NUMA 拓扑（numactl --hardware）+ 本地/远程分配 + 进程绑节点 | `bash -n` ✓；numactl/numastat 本机已装 |
| [mem-observability.sh](./scripts/mem-observability.sh) | 7.5 块5 | swap 监控 / major-faults 缺页热点火焰图 / drsnoop 直接回收受害者 | `bash -n` ✓；drsnoop-bpfcc 已装（需 root） |
| [mem-tuning.sh](./scripts/mem-tuning.sh) | 7.6 块6/7 | swapoff 操作指引 / LD_PRELOAD TCMalloc 运行 | `bash -n` ✓；libtcmalloc 本机**未装**（`sudo apt install libgoogle-perftools4`） |

## 运行前提

- 已装：numactl、numastat、slabtop、drsnoop-bpfcc
- 未装：libtcmalloc（仅 mem-tuning.sh tcmalloc 子命令需要）
- swapoff/slabtop/drsnoop 段需 root
