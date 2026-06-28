# Ch 11 §5 DCCP · Datagram Congestion Control Protocol

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 5. DCCP (Datagram Congestion Control Protocol)

**DCCP** — **不可靠数据报 + 拥塞控制 + 面向连接**；介于 **UDP（无拥塞控制）** 与 **TCP（可靠）** 之间。目标应用：**VoIP、流媒体** — **允许丢包、忌 bufferbloat**。

---

## 设计定位

| 特性 | DCCP |
|------|------|
| 消息边界 | ✓（类 UDP） |
| 可靠交付 | ✗ |
| 拥塞控制 | ✓（类 TCP，多种 CCID） |
| 连接 | **3 次握手**（DCCP-Request/Response） |

**CCID：** **CCID2**（TCP-like AIMD）、**CCID3**（TFRC）— 协商 **拥塞算法**。

---

## 头部

**可变长头部** — 含 **源/目的端口**、**序列号**、**服务码 Service Code**；支持 **扩展选项**（如 **扩展序列号** 防 wrap）。

---

## 内核实现概要

| 组件 | 说明 |
|------|------|
| **`dccp_v4_init()`** | 注册 protocol handler |
| **`dccp_sendmsg()` / `dccp_rcv()`** | 收发 |
| **`SOCK_DCCP`** | 专用 socket 类型 |

**状态机：** **REQUEST → RESPOND → OPEN → CLOSING** — 比 UDP 重，比 TCP 轻（无重传缓冲）。

---

## NAT 穿越

**问题：** DCCP **非 TCP/UDP** — 许多 **CPE/NAT 不支持** 或 **错误映射**。

| 手段 | 说明 |
|------|------|
| **静态 pinhole** | 手工端口映射 |
| **STUN 类探测** | 检测 NAT 行为 |
| **降级 UDP/TCP** | 生产常见 |

与 [Ch 9 NAT](../chapter-09-netfilter/notes/section-5-网络地址转换-NAT.md) **conntrack helper** 问题 **同构** — **非常规 L4** 在 NAT 后 **易碎**。

---

## 本章小结

| 节 | 带走 | HFT |
|----|------|-----|
| §1 | **`socket`/`sock`、msghdr、sk_data_ready** | 🔴 |
| §2 | **`udp_sendmsg`/`udp_rcv`、组播** | 🔴 行情 |
| §3 | **TCP 头/状态机/定时器、`tcp_sendmsg`/`tcp_v4_rcv`** | 🔴 发单 |
| §4 | SCTP 多流/多宿主 | 🟡 |
| §5 | DCCP 不可靠+CC | ⚪ |

---

## 相关章节

- 下一章：[Ch 12 无线](../../chapter-12-无线网络.md)
- NAPI/调优：[Ch 14 高级主题](../../chapter-14-高级主题.md)
- 组播：[note-组播IGMP](../../note-组播IGMP.md)

---

← [4. SCTP](./section-4-SCTP.md) · [Ch 11](../README.md)
