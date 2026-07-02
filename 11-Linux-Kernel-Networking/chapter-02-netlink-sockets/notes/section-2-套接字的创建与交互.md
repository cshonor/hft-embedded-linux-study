# Ch 2 §2 套接字的创建与交互 · Creating Sockets & Interaction

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 2. 套接字的创建与交互 (Creating Sockets & Interaction)

Netlink 在 API 层面 **复用 BSD socket 抽象** — 用户态与内核态各有一套创建入口，但 **收发包语义一致**（`sendmsg`/`recvmsg` + Netlink 地址）。

---

## 用户空间

```c
int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
// 或 SOCK_DGRAM（部分协议）
```

典型流程：

```
socket() → bind(可选，指定 nl_pid / 订阅多播组)
         → sendmsg() 发请求（带 nlmsghdr + 属性）
         → recvmsg() 收回复或异步事件
         → close()
```

**推荐库（原书）：**

| 库 | 说明 |
|----|------|
| **`libnl` / `libnl-3`** | `libnl-route-3`、`libnl-genl-3` — 完整封装 rtnetlink / genl |
| **`libmnl`** | 轻量，iproute2 内部风格，适合 **手写小工具** |

手写 Netlink 易错点：**对齐 padding**、**multipart 消息**（`NLM_F_MULTI` + 末尾 `NLMSG_DONE`）、**序列号** 与请求-回复配对 — 生产环境 **优先 libnl**。

---

## 内核空间

内核子系统通过 **`netlink_kernel_create()`**（3.9 时代 API；现代树有 `__netlink_kernel_create` 等变体）注册 **Netlink 协议处理函数**：

```
netlink_kernel_create(net, NETLINK_ROUTE, cfg)
  → 收到用户消息时调用 .input 回调
  → 用 netlink_unicast / netlink_broadcast 回用户
```

**与 Ch 1 关系：** rtnetlink 改 **`net_device` / FIB`** 的配置面 — **数据面收发包仍走 sk_buff**，Netlink 只 **改控制状态**。

---

## 用户态 vs 内核态对照

| | 用户空间 | 内核 |
|---|----------|------|
| 创建 | `socket(AF_NETLINK, …)` | `netlink_kernel_create()` |
| 发送 | `sendmsg` → 系统调用 | `netlink_unicast` / `broadcast` |
| 地址 | `struct sockaddr_nl` | 目标 `portid` / 多播组 |
| 解析 | libnl 或自解析 `nlmsghdr` | `nlmsg_parse()` 等 |

→ 用户态 socket 总论：[09-UNP](../../09-UNP-Vol1/) · 附录 A：[appendix-A-Linux-API.md](../../appendix-A-Linux-API.md)

---

← [1. 基础与优势](./section-1-Netlink基础与优势.md) · [Ch 2](../README.md) · 下一节 [3. 消息格式](./section-3-数据结构与消息格式.md)
