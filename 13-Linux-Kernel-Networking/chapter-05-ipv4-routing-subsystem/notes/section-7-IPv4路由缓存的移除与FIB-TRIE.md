# Ch 5 §7 IPv4 路由缓存的移除与 FIB TRIE · Removal of IPv4 Routing Cache

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 7. IPv4 路由缓存的移除 (Removal of IPv4 Routing Cache)

**Linux 3.6** 移除独立 **IPv4 routing cache**（曾 **per-flow** 缓存 `dst`）— 原因：**DoS 内存耗尽**；替代：**FIB TRIE** LPM + **更轻量的 dst 绑定**。

---

## 旧 routing cache 的问题

```
攻击：向大量随机目的发包
  → 每 (src,dst,… ) 创建 routing cache 项
  → 内存暴涨、查找退化 — DoS
```

| 旧模型 | 问题 |
|--------|------|
| **Per-flow cache** | 流越多 **条目越多** — 与 **连接数** 绑定 |
| **GC 压力** | 高 PPS 随机 dest → **cache 抖动** |

---

## 3.6+ 模型：FIB TRIE

**TRIE（字典树）** 存 **前缀 LPM** — 查找成本 **与表大小相关**，**不随并发流数线性爆炸**。

```
fib_table
  └─ fib_trie（或 hash 辅助结构，随版本演进）
        └─ 前缀节点 plen 32, 24, 16 …
              └─ fib_alias → fib_info
```

| 对比 | Routing cache 时代 | FIB TRIE 时代 |
|------|-------------------|---------------|
| 主存储 | FIB + **巨大 flow cache** | **FIB 为主** |
| 查找 | cache miss → FIB | **直接 FIB LPM** |
| DoS 面 | **随机 dest 打满 cache** | 仍可能有 **orphan dst**，但 **无 per-random-host 流表** |
| 性能 | 命中快 / miss+GC 痛苦 | **可扩展** 于 **大 FIB**（运营商） |

**3.9（本书）：** 已处于 **TRIE 稳定期** — Rosen 本章基于此后台叙述。

---

## 现代延伸（读 3.9 之后）

| 特性 | 说明 |
|------|------|
| **FIB multipath** | 单前缀 **多 nh ECMP** |
| **BPF fib lookup** | 可编程选路 |
| **`ipv4_fib_lookup()` 重构** | 子系统持续瘦身 |

**HFT 共置：** FIB 条目 **几十条级** — LPM **纳秒级** 非瓶颈；瓶颈仍在 **L4、拷贝、PCIe**（Ch 11/14）。

---

## 本章小结

| 节 | 带走 |
|----|------|
| §1 | **FIB、转发不经 L4、default route** |
| §2 | **`flowi4` → fib_lookup → dst input/output** |
| §3 | **table 254/255、fib_info、alias、nh** |
| §4 | **PMTU/redirect 动态 exception** |
| §5 | **ip rule + 255 tables** |
| §6 | **Redirect 生成/接收、安全关闭** |
| §7 | **3.6 去 routing cache → TRIE** |

---

## 相关章节

- 下一章：[Ch 6 高级路由](../../chapter-06-高级路由.md)
- IPv4 包路径：[Ch 4](../../chapter-04-ipv4/)

---

← [6. ICMP 重定向](./section-6-ICMPv4重定向消息.md) · [Ch 5](../README.md)
