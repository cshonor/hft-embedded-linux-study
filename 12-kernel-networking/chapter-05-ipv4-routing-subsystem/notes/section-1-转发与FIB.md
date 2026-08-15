# Ch 5 §1 转发与 FIB · Forwarding and the FIB

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 1. 转发与 FIB (Forwarding and the FIB)

**路由** 的核心任务：根据 **转发表 (FIB)** 决定 **下一跳 / 出接口**，把包从 **一个子网送到另一个子网**。

---

## 转发路由器上的路径

```
L2 收包 → L3 查 FIB → 直接 L2 发出
              ↑
         不上升到 L4（无 TCP/UDP 处理）
```

| 对比 | 本地交付 | 转发 |
|------|----------|------|
| 目的 | 本机地址 | 非本机，经本机 relay |
| 后续 | `ip_local_deliver` → L4 | **`ip_forward`** → 出接口 |
| CPU | 协议栈全深 | **仅 L3+L2** — 路由器设计目标 |

→ 转发细节：[Ch 4 §7](../../chapter-04-ipv4/notes/section-7-数据包转发.md)

---

## FIB 是什么

**Forwarding Information Base** — 内核中 **「目的前缀 → 如何到达」** 的表项集合，与用户态 **`ip route show`** 对应（经 **rtnetlink** 配置，[Ch 2](../chapter-02-netlink-sockets/)）。

| 用户态 | 内核 |
|--------|------|
| `ip route add 10.0.0.0/8 via 192.168.1.1 dev eth0` | **fib_table** 插入前缀 + **nexthop** |
| `ip route show table main` | **table 254** |

---

## 默认网关 / 默认路由

**0.0.0.0/0** — **匹配所有** 无更具体前缀的目的地址。

```bash
ip route add default via 192.168.1.1 dev eth0
# 等价于 0.0.0.0/0 nexthop
```

**HFT 共置：** 常 **静态 default + 静态行情/发单前缀** — 避免运行 **BGP/OSPF 守护进程** 引入抖动；FIB **只读化** 利于排障。

---

## 与 RIB 的区分（概念）

| 术语 | 层 | 说明 |
|------|-----|------|
| **RIB** | 控制平面 | 路由协议/用户配置 **全量** 路由知识 |
| **FIB** | 数据平面 | **转发实际查** 的表 — Linux 内核 **fib_* 即 FIB** |

本书语境下 **「路由表」= FIB**。

---

← [Ch 5](../README.md) · 下一节 [2. 路由查找](./section-2-路由子系统查找.md)
