# Ch 4 §3 接收 IPv4 多播数据包 · Receiving IPv4 Multicast Packets

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 3. 接收 IPv4 多播数据包 (Receiving IPv4 Multicast Packets)

目的地址 **224.0.0.0/4** 的包 **不能** 简单按单播 FIB 处理 — 需判断：**本机是否加入该组**、**是否为多播路由器**。

---

## 与单播的差异

| | 单播 | 多播 |
|---|------|------|
| 路由 | `ip_route_input()` | **`ip_route_input_mc()`** |
| 交付条件 | 目的 = 本机地址 | **IGMP 加入** 或 **mr 配置** |
| 转发 | 可选 `ip_forward` | **`ip_mr_input()`** + **MFC** |

---

## 关键函数

```
ip_route_input_mc()
  → 查接口、IGMP 组 membership、是否 CONFIG_IP_MROUTE
  → 决定 LOCAL / FORWARD / DROP

ip_mr_input()          /* 多播路由输入 */
  → 查 MFC (Multicast Forwarding Cache)
  → 复制到多个出接口或送本地
```

**MFC (Multicast Forwarding Cache)：** 缓存 **(S,G) 或 (*,G)** → 出接口列表，避免每包全表查 **multicast routing table**。

---

## 与 IGMP / 用户态

主机加入组：**IGMP Report** — 内核 **`igmp.c`** 维护 **`ip_mc_list`**（每接口组播过滤器）。

→ 深读：[note-组播IGMP](../../note-组播IGMP.md) · [Ch 11 L4](../../chapter-11-layer-4-protocols/) UDP 组播 socket

---

## HFT 要点

| 主题 | 行动 |
|------|------|
| **组播行情** | `IP_ADD_MEMBERSHIP`、正确 **接口索引**、交换机 **IGMP Snooping** |
| **内核路径** | 仍走 **L2 → ip_rcv → mc 路由 → UDP**；与 **DPDK 组播** 对比 |
| **过滤** | 未 join 的组 **默认丢弃** — 防泛洪 |

---

← [2. 接收路径](./section-2-协议初始化与接收路径.md) · [Ch 4](../README.md) · 下一节 [4. IP 选项](./section-4-IP选项.md)
