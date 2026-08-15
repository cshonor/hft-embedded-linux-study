# 5.3 名称解析

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md)

**核心主旨**：MAC/端口/IP 如何变成可读名称；性能冻结与安全泄露风险及规避。

## 核心知识点

**名称解析（Name Resolution）**：将数字地址转为易读名称（主机名、服务名、厂商名）。

### 三大机制

| 类型 | 设置项（Preferences → Name Resolution） | 行为 |
|------|----------------------------------------|------|
| **MAC** | Resolve MAC addresses | 尝试 ARP 表映射；否则读 `ethers`；或 OUI → **厂商名**（如 `Netgear_01:02:03`） |
| **传输层** | Resolve transport names | 端口 → 服务名（80 → `http`） |
| **网络/IP** | Resolve network (IP) names | IP → 主机名，常依赖 **DNS 反向查询** |

### 性能问题与界面冻结

| 问题 | 原因 |
|------|------|
| 打开大文件极慢、界面卡死 | 默认**顺序**对每个未知 IP 做 DNS 查询并等待 |

| 策略 | 操作 |
|------|------|
| **并发 DNS** | 勾选 **Enable concurrent DNS name resolution** |
| **关闭 IP 解析** | 排障大数据集时关闭 network name resolution |
| **离线分析** | 先关解析快速浏览，需要时再临时开启 |

### 安全泄露风险（高危）

| 场景 | 风险 |
|------|------|
| 分析**恶意流量** | 反向 DNS 查询发到**攻击者控制的 DNS** → **暴露分析师 IP/时间** |

| 策略 | 操作 |
|------|------|
| 关闭外部解析 | 取消 **Use an external network name resolver** |
| 恶意样本 lab | 断网打开 pcap 或仅用 hosts 文件 |

### 自定义 hosts 文件

| 项 | 说明 |
|----|------|
| **优势** | 大量内网 IP 可读；**无外部查询** |
| **格式** | 与系统 hosts 相同：`192.168.1.1    server01` |
| **位置** | Wireshark **个人配置**目录（`Help` → `About` → `Folders`） |
| **易错** | 文件须为**无后缀**纯文本；放错目录不生效 |

## 抓包/实操记录

| 场景 | 建议 |
|------|------|
| 日常内网 | 维护 `hosts` 映射核心服务器 |
| 恶意 pcap | 关闭外部 DNS；必要时全关名称解析 |
| 大文件 | 先关解析打开 → 定位 IP 后再局部开 |

## 疑问与总结

- 名称解析只影响**显示**，不改变包内容。
- Info 列里的主机名可能来自**抓包文件内的 DNS 响应**，不一定来自 Wireshark 事后查询。
