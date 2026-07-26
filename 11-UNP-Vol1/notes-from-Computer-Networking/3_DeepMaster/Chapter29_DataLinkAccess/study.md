# 第 29 章：数据链路访问（厚版）

> [Ch 28](../Chapter28_RawSocket/study.md) · **Ch 29** · 阶段三收束（DeepMaster 主线）  
> 逐节：`29.x_*.md`

> **说明**：上传资料截至第 8 章；第 29 章按 UNP 第 3 版体系整理，请与全本对照验证。

## 本章目标

理解链路层访问场景、**BPF/PF_PACKET/DLPI** 差异、**libpcap/libnet** 范式、DNS 抓包与 **UDP 伪首部校验和**、分片陷阱。

---

## 小节索引

| 节 | 目录 | 主题 |
|----|------|------|
| 29.1 | [29.1_Overview](./29.1_Overview.md) | 二层 vs Ch28 |
| 29.2 | [29.2_BSD_Packet_Filter](./29.2_BSD_Packet_Filter.md) | BPF、/dev/bpf |
| 29.3 | [29.3_Datalink_Provider_Interface](./29.3_Datalink_Provider_Interface.md) | DLPI、STREAMS |
| 29.4 | [29.4_Linux_Packet_Socket](./29.4_Linux_Packet_Socket.md) | PF_PACKET、LSF |
| 29.5 | [29.5_Libpcap_Capture_Lib](./29.5_Libpcap_Capture_Lib.md) | **libpcap** |
| 29.6 | [29.6_Libnet_Packet_Build_Lib](./29.6_Libnet_Packet_Build_Lib.md) | **libnet** |
| 29.7 | [29.7_UDP_Checksum_Check](./29.7_UDP_Checksum_Check.md) | 帧解析、校验和 |
| 29.8 | [29.8_Summary](./29.8_Summary.md) | 全章收束 |

---

## 一章速记

```text
嗅探/ARP/自构以太网帧 → 链路层（无统一 syscall）
BPF：字节码内核过滤；Linux PF_PACKET + SO_ATTACH_FILTER
读：libpcap（pcap_compile/setfilter/loop）
写：libnet（build_ethernet/ip/tcp + write）
勿用 SOCK_PACKET；TCP/UDP 嗅探靠 libpcap 非 raw IP
抓包解析：以太网→IP→UDP；校验和含伪首部；分片仅首片有 UDP 头
```

---

## 与前面章节挂钩

| 章节 | 关联 |
|------|------|
| Ch 28 | IP raw 不收 TCP/UDP |
| Ch 17.8 | ARP 与链路层 |
| Ch 8 | UDP 校验和概念 |
| 链路层课程 | 以太网帧、MAC（仓库内 06_link_layer） |

---

## 3_DeepMaster 进度

| 章 | 状态 |
|----|------|
| 17、20–22、24–25、28–**29** | **厚版完成** |
| 18、23 | 待笔记 |
