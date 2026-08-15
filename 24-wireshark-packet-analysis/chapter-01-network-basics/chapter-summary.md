# 第1章 数据包分析技术与网络基础

> 全书：[../README.md](../README.md)

## 整体框架

```text
1.1 数据包分析 / 嗅探器三步（收集→转换→分析）
        ↓
1.2 协议栈 · OSI · 封装/解封装 · Hub/交换机/路由器
        ↓
1.3 广播 / 组播 / 单播（读包时认地址、估性能影响）
        ↓
1.4 小结 → 抓包前必先懂底层，再谈过滤器与排障
```

| 小节 | 主题 | 文件 |
|------|------|------|
| 1.1 | 嗅探器定义与 Collect→Convert→Analyze | [01-what-is-packet-analysis.md](./01-what-is-packet-analysis.md) |
| 1.2 | OSI、封装、二层/三层设备 | [02-network-communication-basics.md](./02-network-communication-basics.md) |
| 1.3 | 广播 / 组播 / 单播 | [03-traffic-classification.md](./03-traffic-classification.md) |

## 重点难点

| 点 | 说明 |
|----|------|
| **混杂模式** | 才能看到「非发给自己」的帧；无线/虚拟网卡能力不一 |
| **封装方向** | 发送 7→1 加头，接收 1→7 剥头；Wireshark 树 = 接收方视角解封装 |
| **交换机 vs 路由器** | L2 按 MAC 转发 vs L3 按 IP 路由；决定抓包接在哪、能看到谁 |
| **广播 vs 组播** | 前者全网段；后者需加入组；过多广播 → 性能问题 |
| **第 8 层** | 用户误操作，非协议故障 |

## 实操要点

1. 本机抓一段 HTTP/HTTPS，在协议树指出 **帧 / IP / TCP / 应用** 四层对应关系。
2. 用 `arp` 过滤器观察 **广播问、单播答**。
3. 对照 [cheatsheet/notes.md](../cheatsheet/notes.md) 练 `ip.addr`、`tcp.port` 基础过滤。
4. 生产环境大流量：**tcpdump 落盘 + Wireshark 离线分析**。

## 1.4 本章小结

数据包分析要有效，必须建立在对底层原理的理解之上：

- 透彻掌握 **网络通信原理**、**协议层次**（OSI / TCP/IP 对照）、**封装流动规则**；
- 清楚 **Hub / 交换机 / 路由器** 各在哪一层、如何转发；
- 能区分 **广播、组播、单播** 在抓包中的表象与性能含义。

在此基础上，才能在复杂故障中**先定层级、再选工具与过滤器**，避免「只会点 Decode 却读不懂包」。

## 小节索引

- [1.1 数据包分析与数据包嗅探器](./01-what-is-packet-analysis.md)
- [1.2 网络通信原理](./02-network-communication-basics.md)
- [1.3 流量分类](./03-traffic-classification.md)
