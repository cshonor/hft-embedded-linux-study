# 1.9 测试用网络及主机

## 核心知识点

描述书中代码测试用的**物理拓扑**，便于对照日志与路由输出。

## 关键定义与工具

| 项 | 说明 |
|----|------|
| **CIDR** | `/n` 表示前缀长度，如 `172.24.37/24` 前 24 位为网络号 |
| `netstat -ni` | 接口信息：MTU、收发包、错误数；`lo` 回环、`eth0` 以太网 |
| `netstat -nr` | 内核**路由表** |
| `ifconfig eth0` | 单接口地址、广播地址、掩码、MAC 等 |
| `ping -b` | 向子网广播地址发包，发现局域网活跃主机 |

## 易错点与坑点

Solaris / macOS / FreeBSD / Linux 上 `netstat`、`ifconfig` **输出格式不同**，需查 `man` 确认本机行为。

## 个人学习总结

> 💡 可拓展：`ip addr` / `ip route`（iproute2）与 `ifconfig` / `netstat` 对照表。
