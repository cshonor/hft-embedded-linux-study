# 02-ch04 code · BCC 工具箱

> 《BPF Performance Tools》第 4 章示例落盘。
> 笔记中 get_table API（§11 块1）为 Python 引用，已并入 bcc-toolbox.sh 注释。

## scripts/

| 脚本 | 出处 | 内容 | 验证（2026-10 Ubuntu 24.04） |
|------|------|------|------|
| [bcc-env-check.sh](./scripts/bcc-env-check.sh) | §3 块7 | 内核配置八查 + 缺项后果速查 | `bash -n` ✓ 实跑 ✓（本机八项全 =y/m） |
| [bcc-toolbox.sh](./scripts/bcc-toolbox.sh) | §5-8 块8-13 | 瑞士军刀四件：funccount 五式 / stackcount+火焰图 / trace 四式 / argdist + 选型口诀 | `bash -n` ✓；四工具均已装 |
| [bcc-debug.sh](./scripts/bcc-debug.sh) | §12 块2-6 | 调试五连：trace_pipe / `--ebpf` / bpflist / dmesg / verifier log | `bash -n` ✓；opensnoop --ebpf 实跑 ✓ |

## 运行前提

- funccount/stackcount/trace/argdist/opensnoop/bpflist-bpfcc：本机全部已装
- BPF 加载全部需 root
