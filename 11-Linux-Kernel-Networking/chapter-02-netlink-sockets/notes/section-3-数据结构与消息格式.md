# Ch 2 §3 数据结构与消息格式 · Data Structures & Message Format

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 3. 数据结构与消息格式 (Data Structures & Message Format)

Netlink 消息有 **固定头部 + 可变载荷**；载荷几乎总是 **TLV (Type-Length-Value)** 属性链，便于 **向前兼容扩展**。

---

## `sockaddr_nl` — Netlink 地址

```c
struct sockaddr_nl {
    sa_family_t nl_family;   /* AF_NETLINK */
    unsigned short nl_pad;
    u32 nl_pid;              /* 端口 ID：用户态常为 getpid()；0 表示内核 */
    u32 nl_groups;           /* 多播组掩码（订阅哪些组） */
};
```

| 字段 | 含义 |
|------|------|
| **`nl_pid`** | 逻辑「端口」— 区分同一主机上多个 Netlink 客户端 |
| **`nl_groups`** | **位掩码** — 订阅链路/邻居等 **多播通知** |

---

## `nlmsghdr` — 消息头

每条 Netlink 消息以 **`struct nlmsghdr`** 开头：

| 字段 | 作用 |
|------|------|
| **`nlmsg_len`** | 整条消息长度（含头） |
| **`nlmsg_type`** | 消息类型（如 `RTM_NEWROUTE`、`RTM_NEWLINK`） |
| **`nlmsg_flags`** | `NLM_F_REQUEST`、`NLM_F_ACK`、`NLM_F_MULTI` 等 |
| **`nlmsg_seq`** | 序列号 — 匹配请求/响应 |
| **`nlmsg_pid`** | 发送方 port ID |

```
┌──────────────────────────────────────┐
│  nlmsghdr                            │
├──────────────────────────────────────┤
│  载荷：固定 struct（如 ifinfomsg）     │
├──────────────────────────────────────┤
│  属性区：TLV 嵌套（rtattr / nla）      │
│    [type][len][payload] [type][len]… │
└──────────────────────────────────────┘
```

---

## TLV 属性

**Type-Length-Value** — 长度 **`nla_len`** 含头部，**4 字节对齐**：

| 优点 | 说明 |
|------|------|
| **可扩展** | 新老内核可忽略未知 type |
| **可嵌套** | `NLA_NESTED` — 复杂结构（如 tc 规则） |
| **统一解析** | `nla_parse()` / libnl 属性 API |

rtnetlink 示例：`RTM_NEWLINK` 后接 **`IFLA_IFNAME`、`IFLA_MTU`、`IFLA_ADDRESS`** 等属性。

---

## 标志位速查

| 标志 | 含义 |
|------|------|
| `NLM_F_REQUEST` | 这是请求，需要 ACK 或数据回复 |
| `NLM_F_ACK` | 请求对方发 `NLMSG_ERROR` 确认 |
| `NLM_F_MULTI` | 多条回复之一；末尾 `NLMSG_DONE` |
| `NLM_F_CREATE` | 创建对象（路由/邻居等） |

**排查：** `NLMSG_ERROR` 里 **`errno`** 解释 `ip route` 失败原因 — 比 ioctl 的 `-1` 更清晰。

---

← [2. 创建与交互](./section-2-套接字的创建与交互.md) · [Ch 2](../README.md) · 下一节 [4. Generic Netlink](./section-4-通用Netlink协议.md)
