# Ch 6 §2 策略路由 · Policy Routing

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 2. 策略路由 (Policy Routing)

**策略路由** 使选路 **不限于目的 IP** — 还可按 **源地址、TOS、mark、入接口** 等选 **不同 FIB 表**。概念见 [Ch 5 §5](../../chapter-05-ipv4-routing-subsystem/notes/section-5-策略路由.md)；本节侧重 **`fib_rules` 基础设施** 与 **默认三表**。

---

## 用户态管理 — `ip rule`

```bash
ip rule show                    # 优先级 + 条件 + lookup table
ip rule add from 10.0.0.0/8 table 100 priority 100
ip rule del priority 100
ip route show table 100
```

| 命令 | 作用 |
|------|------|
| **`ip rule add/del`** | 增删 **策略规则** |
| **`ip rule show`** | 列出 **priority 排序** 的规则链 |
| **`ip route … table N`** | 维护 **表 N 的 FIB** |

最多 **255** 张 **用户自定义表**（+ 系统保留表）。

---

## 内核：`fib_rules` 模块

```
包需路由 → fib_rules_lookup(fl4, …)
  → 按 priority 遍历 rule
       ├─ 匹配 from/to/tos/mark/iif…
       └─ 命中 → fib_lookup(指定 table_id)
  → 无命中 → 默认 **main (254)**
```

| 组件 | 说明 |
|------|------|
| **`struct fib_rule`** | 单条策略（选择器 + table + priority） |
| **`fib_rules_lookup()`** | **Rx/Tx** 查表前 **先过 rule** |
| **与 Netlink** | `RTM NEWRULE/DELRULE` — [Ch 2](../../chapter-02-netlink-sockets/) |

---

## 启动默认三表（策略语境）

| 表 ID | 名 | 角色 |
|-------|-----|------|
| **255** | **local** | 本机地址、广播、nat 环回 — **最高优先级 rule 常指向** |
| **254** | **main** | 管理员 `ip route` 默认写入 |
| **253** | **default** | **兜底** — 无 main 匹配时可 fall back（视发行版 rule 集） |

**`ip rule show` 典型前几行：**

```
0:      from all lookup local
32766:  from all lookup main
32767:  from all lookup default
```

**local 先于 main** — 保证 **127.0.0.1、本机 IP** 走 **local 表**，不被 default GW 拐走。

---

## 与 Ch 5 的分工

| 章节 | 侧重 |
|------|------|
| **Ch 5 §5** | 策略路由 **概念**、HFT **分流场景** |
| **Ch 6 §2** | **`fib_rules`、默认三表、`ip rule` 运维** |

---

## HFT 实践

```
# 例：发单源 10.10.0.5 走交易表
ip rule add from 10.10.0.5 table trading priority 100
ip route add default via 192.168.100.1 dev eth1 table trading

# 行情/mark 分流：iptables/nft mark + ip rule fwmark
```

- 规则 **宜少** — 每包 **线性扫 rule**（规则数小则可忽略）。
- **`ip rule`/`ip route` 变更** 与 **热路径** 无 syscall，但 **影响所有新 lookup**。

→ Netfilter mark：[Ch 9](../../chapter-09-netfilter/)

---

← [1. 多播路由](./section-1-多播路由.md) · [Ch 6](../README.md) · 下一节 [3. 多路径路由](./section-3-多路径路由.md)
