# TCP/IP Illustrated Vol.1 — Stevens（外部仓库）

**定位：** 协议语义与首部格式 · 回答「线上包长什么样」。

**文件夹 04 · 外部书目 外A** · [返回总清单](../READING-LIST.md#外部书目笔记在另一仓库--本仓库仅索引)

## 笔记仓库（外部）

**仓库：** [cshonor/Computer-Networking](https://github.com/cshonor/Computer-Networking)

| 入口 | 链接 |
|------|------|
| TCP/IP 卷1 笔记目录 | [TCP-IP-Volume1-Protocols/](https://github.com/cshonor/Computer-Networking/tree/main/TCP-IP-Volume1-Protocols) |
| 章节目录 | [OUTLINE.md](https://github.com/cshonor/Computer-Networking/blob/main/TCP-IP-Volume1-Protocols/OUTLINE.md) |
| 速查 | [QUICKREF.md](https://github.com/cshonor/Computer-Networking/blob/main/TCP-IP-Volume1-Protocols/QUICKREF.md) |

## HFT 必读 / 选读 / 跳过

| 原书章节 | 标签 | HFT 关联 |
|----------|------|----------|
| Ch 7 广播与多播、IGMP | 🔴 必读 | 交易所行情组播 |
| Ch 8 UDP：首部、长度、校验 | 🔴 必读 | 行情包解析 |
| Ch 3 IP：分片、DF、TTL | 🟡 选读 | 避免 IP 分片增延迟 |
| Ch 9–11 TCP：握手、窗口、超时、拥塞 | 🟡 选读 | **订单走 TCP 时升为必读** |
| Ch 6 ICMP | 🟡 选读 | 网络排查 |
| Ch 17–18 路由 | 🟡 选读 | 托管/共置 |
| Ch 2 链路层、ARP | ⚪ 跳过 | 除非 DPDK L2 / raw socket |
| Ch 14 DNS、Ch 15–16 SNMP/HTTP | ⚪ 跳过 | 非热路径 |

## 为何不在本仓库展开

笔记已在你的网络书仓库维护；本仓库只做 HFT 裁剪索引与阅读顺序编排。

## 交叉阅读

- API 层 → [05-UNP-Vol1](../05-UNP-Vol1/)
- 内核实现 → [06-Linux-Kernel-Networking](../06-Linux-Kernel-Networking/)
- 调优观测 → [01-Systems-Performance-2nd](../01-Systems-Performance-2nd/)、[09-BPF-Performance-Tools](../09-BPF-Performance-Tools/)
