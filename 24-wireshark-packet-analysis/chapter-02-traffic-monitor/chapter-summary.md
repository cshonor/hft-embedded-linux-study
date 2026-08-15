# 第2章 监听网络线路

> 全书：[../README.md](../README.md) · 上一章：[第1章 网络基础](../chapter-01-network-basics/chapter-summary.md)

## 整体框架

```text
2.1 混杂模式（驱动层：能收「非本机 MAC」的帧）
        ↓
2.2 Hub 环境（天然广播，已少见）
        ↓
2.3 交换机环境（镜像 / Hub out / TAP / ARP 污染）
        ↓
2.4 路由多网段（抓包点决定看见请求还是应答）
        ↓
2.5 五种方案选型与无痕原则
```

| 小节 | 主题 | 文件 |
|------|------|------|
| 2.1 | 混杂模式与权限 | [01-promiscuous-mode.md](./01-promiscuous-mode.md) |
| 2.2 | Hub 嗅探与碰撞 | [02-sniff-on-shared-network.md](./02-sniff-on-shared-network.md) |
| 2.3 | 交换机四种扩视手段 | [03-sniff-on-switched-network.md](./03-sniff-on-switched-network.md) |
| 2.4 | 路由/多网段抓包点 | [04-sniff-on-routed-network.md](./04-sniff-on-routed-network.md) |
| 2.5 | 部署选型指南 | [05-deployment-guidelines.md](./05-deployment-guidelines.md) |

## 重点难点

| 点 | 说明 |
|----|------|
| **混杂 ≠ 全看见** | 交换机仍可能不转发他人单播到本口 |
| **镜像过载** | 多口全双工镜像到一口 → 丢包、交换机异常 |
| **真 Hub vs 假 Hub** | Hub out 前必须验证设备 |
| **ARP 污染** | MITM、瓶颈、DoS；非日常首选 |
| **路由边界** | 源网段只见请求不见应答 → 上移抓包点，勿误判服务器 |
| **本机抓包** | 排障证据力弱，OS/栈故障会扭曲现象 |

## 实操要点

1. Wireshark 捕获选项确认 **Promiscuous**；Linux 可用 `ip link set promisc on` 验证。
2. 实验：交换机普通口 vs 镜像口对比同一对话可见性。
3. 画拓扑后再抓；跨网段时在**网关两侧**各抓一段对照。
4. 选型默认：**镜像 > TAP > Hub out >> ARP 污染**；见 [05-deployment-guidelines.md](./05-deployment-guidelines.md)。

## 小节索引

- [2.1 混杂模式](./01-promiscuous-mode.md)
- [2.2 在集线器连接网络中嗅探](./02-sniff-on-shared-network.md)
- [2.3 在交换式网络中进行嗅探](./03-sniff-on-switched-network.md)
- [2.4 在路由网络环境中进行嗅探](./04-sniff-on-routed-network.md)
- [2.5 部署嗅探器的实践指南](./05-deployment-guidelines.md)
