# ch15 code · BPF/BCC/bpftrace 示例

> 本章笔记中的程序类代码块补全/落盘；说明性伪代码块保留在笔记内。
> BPF 纵深（两本书 + 真机实验）在 [06.7-bpf-observability](../../../06.7-bpf-observability/)，本章只是入口。

## 程序

| 文件 | 出处 | 内容 | 验证（2026-10 Ubuntu 24.04） |
|------|------|------|------|
| [decode-latency.bt](./decode-latency.bt) | 15.1.7 块1、15.2 块6 | uprobe/uretprobe 用户态函数延迟直方图（参数化二进制与函数名，含 10s 周期打印） | bpftrace 语义检查 ✓（实跑需 root） |
| [bcc_skeleton.py](./bcc_skeleton.py) | 15.1 块3 | 笔记伪代码补全为可运行 BCC 骨架：BPF_HASH + kprobe + 定时读 map | `py_compile` ✓；BCC import ✓（attach 需 root） |

## scripts/

| 脚本 | 出处 | 内容 | 验证 |
|------|------|------|------|
| [bcc-tools-quick.sh](./scripts/bcc-tools-quick.sh) | 15.1 块2 | BCC 成品四连：runqlat/biolatency/tcplife/offcputime + 选型口诀 | `bash -n` ✓；四个工具均已装 |
| [bpftrace-oneliners.sh](./scripts/bpftrace-oneliners.sh) | 15.2 块4/5/7 | 单行集：syscall 计数/execve 追踪/心跳/块 I/O 直方图/vfs top/profile:hz:99 + `-l` 探针勘探 | `bash -n` ✓；bpftrace -l 勘探段实跑 ✓ |

## 运行前提

- bpftrace 0.20.2、bpfcc-tools 0.29.1、python3-bcc 0.29.1：本机均已装
- **BPF 程序加载/attach 全部需要 root**（本机未实测执行，仅语法/语义级验证）
- 06.7 模块的 XDP 程序、CO-RE 编译在 [06.7-bpf-observability](../../../06.7-bpf-observability/)
