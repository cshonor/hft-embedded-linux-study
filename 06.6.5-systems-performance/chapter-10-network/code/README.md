# ch10 code · 网络排查脚本

> 本章笔记中的命令序列落盘；说明性伪代码块保留在笔记内。
> 网络纵深（NAPI/RSS/组播收包路径）在 [12.5-modern-networking](../../../12.5-modern-networking/)，DPDK 侧在 [13-dpdk](../../../13-dpdk/)。

## scripts/

| 脚本 | 出处 | 内容 | 验证（2026-10 Ubuntu 24.04） |
|------|------|------|------|
| [net-triage.sh](./scripts/net-triage.sh) | 10.5 块1、10.6 块2/3 | 丢包排查三连：ethtool -S（rx_missed 等）/ softnet_stats backlog / 抓包守则（filter+限量+限时） | `bash -n` ✓；softnet_stats 段实跑 ✓ |
| [netem-lab.sh](./scripts/netem-lab.sh) | 10.7-10.8 块4 | iperf3 4 流吞吐 / tc netem 损伤注入（延迟+丢包+乱序）与撤销 | `bash -n` ✓；iperf3 本机**未装**；tc 段需 root 且默认只打印命令（防误伤生产网卡） |

## 运行前提

- 已装：ethtool、tcpdump、tc（iproute2）
- 未装：`iperf3`（`sudo apt install iperf3`）
- netem 损伤注入是**测试环境专用**，脚本默认只打印命令不执行
