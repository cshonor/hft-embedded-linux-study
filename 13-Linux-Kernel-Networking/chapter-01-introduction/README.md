# Ch 1 简介 · Introduction

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

> 本章 **不逐行抠代码**，而是给出 **Linux 内核网络子系统** 的全景：各层在做什么、**`net_device` / `sk_buff`** 两大基石、**收发包在栈内怎么流转**，以及 **git + netdev 邮件列表** 的内核网络开发模式。  
> 代码基准：**Linux kernel 3.9**（后文各章会标注与现代 5.x/6.x 的差异）。

---

## 本章概述

| | 内容 |
|---|------|
| **本章** | L2–L4 栈概览 · 核心数据结构 · 路由/Netfilter 路径 · 开发流程 |
| **Ch 2 起** | Netlink、ICMP、IPv4/路由、邻居、Netfilter 细节、L4 TCP/UDP 等 **专题深读** |
| **HFT 读法** | 带走 **「一条包从网卡到 socket 经过谁」** 的地图；细节在 Ch 11 / Ch 14 / DPDK 对照 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. Linux 网络栈 | [notes/section-1-Linux网络栈.md](./notes/section-1-Linux网络栈.md) |
| 2. 网络设备 `net_device` | [notes/section-2-网络设备-net_device.md](./notes/section-2-网络设备-net_device.md) |
| 3. 套接字缓冲区 `sk_buff` | [notes/section-3-套接字缓冲区-sk_buff.md](./notes/section-3-套接字缓冲区-sk_buff.md) |
| 4. 数据包的收发与流转 | [notes/section-4-数据包的收发与流转.md](./notes/section-4-数据包的收发与流转.md) |
| 5. Linux 内核网络开发模式 | [notes/section-5-Linux内核网络开发模式.md](./notes/section-5-Linux内核网络开发模式.md) |

---

## 相关章节

- 下一章：[../chapter-02-netlink-sockets/](../chapter-02-netlink-sockets/)
- 传输层深读：[../chapter-11-layer-4-protocols/](../chapter-11-layer-4-protocols/)
- 高级主题（NAPI/softirq/RSS）：[../chapter-14-高级主题.md](../chapter-14-高级主题.md)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
