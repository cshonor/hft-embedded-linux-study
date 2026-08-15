# 5.1 端点和网络会话

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · 基线：[§4.1 捕获文件](../chapter-04-capture-packet/01-use-capture-files.md)

**核心主旨**：从宏观统计**端点**与**会话**，快速找出高流量来源与异常交互对象。

## 核心知识点

### 关键定义

| 术语 | 说明 |
|------|------|
| **端点（Endpoint）** | 能发送/接收数据的设备；不同层用不同地址标识 |
| **会话（Conversation）** | 两个端点之间的通信过程；Wireshark 按协议层地址对标识 |

| OSI 层 | 端点地址示例 |
|--------|----------------|
| L2 | MAC |
| L3 | IPv4 / IPv6 |
| L4 | IP + 端口（TCP/UDP 会话） |

### 1. 端点统计（Endpoints）

`Statistics` → `Endpoints`

| 功能 | 说明 |
|------|------|
| 列表字段 | 地址、发送/接收**包数**、**字节数** |
| 协议选项卡 | Ethernet、IPv4、TCP、UDP 等分层查看 |
| **Limit to display filter** | 仅统计当前**显示过滤器**后的流量 |

**实战捷径**：端点行 **右键** → 创建显示过滤器 / **Colorize**（着色）→ 在列表中视觉追踪该主机。

### 2. 网络会话（Conversations）

`Statistics` → `Conversations`

| 功能 | 说明 |
|------|------|
| 展示 | Address A ↔ Address B 的包数、字节数（常分 A→B / B→A） |
| 用途 | 看「谁在和谁说话」、是否单向流量异常 |

### 3. 定位最高用量者（Top Talkers）

| 步骤 | 操作 |
|------|------|
| 1 | Endpoints 或 Conversations 窗口 |
| 2 | 按 **Bytes** 列降序排序 |
| 3 | 锁定产生巨大流量的主机或会话对 |

**外部验证**：未知公网 IP → **WHOIS**（ARIN、RIPE、APNIC 等）或 Robtex → 确认是否 CDN/视频（如 Google/YouTube）等业务流量，避免误当攻击。

> **拓展**：`Preferences` 配置 **GeoIP** 数据库可在端点列表显示 IP 地理位置（需定期更新 MaxMind 等库）。

## 抓包/实操记录

| 练习 | 目标 |
|------|------|
| Top Talker | 抓 5 分钟上网流量 → Endpoints IPv4 按 Bytes 排序 → 记录第一名 |
| 会话对 | Conversations TCP → 找字节最大的 A↔B → 显示过滤器右键追踪 |
| 与基线比 | 对比 [baseline pcap](../chapter-03-wireshark-intro/04-get-started.md) 中同一主机的 Bytes 占比 |

## 疑问与总结

- Endpoints = **单地址**汇总；Conversations = **地址对**汇总。
- 务必勾选 **Limit to display filter** 时，先确认主窗口过滤器意图，否则统计范围不符预期。
