# Ch 9 §2 Netfilter 钩子 · Netfilter Hooks

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 2. Netfilter 钩子 (Netfilter Hooks)

IPv4/IPv6 **共用 5 个 hook 编号** — 在固定栈位置调用 **`NF_HOOK()`**，按 **priority** 链式执行已注册回调。

---

## 五个核心钩子

| Hook | 时机 | 典型用途 |
|------|------|----------|
| **`NF_INET_PRE_ROUTING`** | **路由前**，所有 **入站** 包 | **DNAT**、raw 表、mangle、TC ingress |
| **`NF_INET_LOCAL_IN`** | 路由判定 **交付本机** 后 | **filter INPUT**、连接跟踪入向 |
| **`NF_INET_FORWARD`** | 路由判定 **转发** | **filter FORWARD** |
| **`NF_INET_LOCAL_OUT`** | **本机产生** 的包，路由后 | **filter OUTPUT**、raw OUTPUT |
| **`NF_INET_POST_ROUTING`** | **离开本机前**（本地+转发） | **SNAT**、mangle、mark |

---

## 路径与钩子顺序（IPv4 简图）

**入站 → 本机：**
```
PRE_ROUTING → 路由 → LOCAL_IN → L4
```

**入站 → 转发：**
```
PRE_ROUTING → 路由 → FORWARD → POST_ROUTING → 出接口
```

**本机发出：**
```
LOCAL_OUT → 路由 → POST_ROUTING → 出接口
```

→ 与 [Ch 1 §4](../../chapter-01-introduction/notes/section-4-数据包的收发与流转.md)、[Ch 4 §2/§7](../../chapter-04-ipv4/) 对照。

---

## 注册：`nf_hook_ops`

```c
static struct nf_hook_ops my_ops = {
    .hook     = my_hook_fn,
    .pf       = NFPROTO_IPV4,      /* 或 IPV6 */
    .hooknum  = NF_INET_PRE_ROUTING,
    .priority = NF_IP_PRI_FILTER,  /* 数字越小越早 */
};
nf_register_net_hook(net, &my_ops);
```

**优先级：** 同 hook 上 **多个模块** 按 priority **排序** — **conntrack 早于 NAT**（§5）。

---

## Verdict（5 种判定）

| 返回值 | 含义 |
|--------|------|
| **`NF_DROP`** | 丢弃，不再传播 |
| **`NF_ACCEPT`** | 继续下一个 hook / 栈 |
| **`NF_STOLEN`** | 模块 **接管 skb**（栈不再处理） |
| **`NF_QUEUE`** | 交 **用户态** 队列（旧 nfqueue） |
| **`NF_REPEAT`** | 重新从 **当前 hook** 执行 |

**HFT 排查：** 包「莫名消失」— **`iptables -t raw -vnL`**、**`nft monitor`**、**trace**（`nft trace` / `iptables -j TRACE`）。

---

← [1. 框架](./section-1-基于Netfilter的框架.md) · [Ch 9](../README.md) · 下一节 [3. 连接跟踪](./section-3-连接跟踪.md)
