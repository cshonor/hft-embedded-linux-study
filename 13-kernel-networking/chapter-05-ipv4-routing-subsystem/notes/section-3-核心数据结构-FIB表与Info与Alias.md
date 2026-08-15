# Ch 5 §3 核心数据结构 · FIB Tables, FIB Info & FIB Alias

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 3. 核心数据结构：FIB 表、FIB Info 与 FIB Alias

FIB 在内存中 **分层组织** — **表 (table)** 装 **前缀项**，具体 **下一跳/指标** 在 **fib_info**，**TOS 变体** 用 **fib_alias** 共享 info 以 **省内存**。

---

## `fib_table` — 路由表

| ID | 名 | 用途 |
|----|-----|------|
| **254** | **main** | 常规 `ip route` 默认表 |
| **255** | **local** | **本机地址/广播** — 自动维护 |
| 1–252 | 自定义 | **策略路由** 引用（§5） |

无策略路由时，**绝大多数 lookup 在 main + local** 完成。

```bash
ip route show table main
ip route show table local
```

---

## `fib_info` — 一条路由的「实质」

同一 **nexthop 集合 + 设备 + 协议 + metrics** 可 **被多条前缀 alias 共享**：

| 字段类 | 含义 |
|--------|------|
| **fib_nh / nexthop** | **网关 IP**、**oif**、权重（multipath） |
| **protocol** | 路由来源：kernel / static / **bird/zebra** 等 |
| **priority** | 度量比较 |
| **metrics** | **MTU**、**advmss**、**initcwnd** 等 — 影响 **PMTU/TCP** |

**HFT：** 静态路由可设 **`mtu lock`** / **`adv_mss`** — 减少 **PMTU 震荡** 对 TCP 的影响。

---

## `fib_alias` — TOS 维度的共享优化

多条路由 **同一 destination**，仅 **TOS** 不同 → **多个 `fib_alias`** 指向 **同一 `fib_info`**：

```
dest 10.0.0.0/8  tos A  ──┐
dest 10.0.0.0/8  tos B  ──┼──► 同一 fib_info（同一 nh、dev）
dest 10.0.0.0/8  tos C  ──┘
```

减少 **重复 nexthop 结构** — 早期 **TOS 路由** 场景；现代 **DSCP 策略路由** 仍可能用到 **mark/tos 选表**（§5）。

---

## `fib_nh` — Nexthop

| 内容 | 说明 |
|------|------|
| **nh_gw** | 下一跳 **网关** IP（直连网可 0） |
| **nh_oif** | **出接口** index |
| **nh_flags** | 如 **ONLINK** |

**多路径 (ECMP)：** 多个 **fib_nh** 挂在同一 **fib_info** — 内核 **hash 选路**（Ch 6 高级路由）。

---

## 关系一图

```
fib_table (254 main)
  └─ trie/ hash 节点：前缀 192.168.0.0/16
        └─ fib_alias (key: tos, type, scope)
              └─ fib_info
                    ├─ fib_nh[0]: via 10.0.0.1 dev eth0
                    └─ metrics: mtu, …
```

---

← [2. 路由查找](./section-2-路由子系统查找.md) · [Ch 5](../README.md) · 下一节 [4. 下一跳异常](./section-4-FIB下一跳异常.md)
