# Ch 9 §6 连接跟踪扩展 · Connection Tracking Extensions

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 6. 连接跟踪扩展 (Connection Tracking Extensions)

**Linux 2.6.23+** — **按需** 为 `nf_conn` 挂 **扩展块**，而非 **所有连接** 背负 **NAT/timestamp/label** 等 **最大字段集** — **省内存**、利 **scale**。

---

## 动机

| 旧思路 | 问题 |
|--------|------|
| **单一巨大 `nf_conn`** | 无 NAT 的连接也 **占 NAT 字段** |
| **百万 conntrack** | 内存 **线性膨胀** |

**扩展机制：** 基础 `nf_conn` **固定小**；模块 **注册 extension id**，仅 **需要时 alloc**。

---

## 常见扩展类型

| 扩展 | 用途 |
|------|------|
| **NAT** | 保存 **original/reply manip** |
| **timestamp** | 规则 **按时间** 匹配 |
| **timeout** | **per-rule 超时** 调整 |
| **labels / secmark** | **SELinux/secmark** 联动 |
| **synproxy / helper** | 专用特性 |

**API 概念：**

```
nf_ct_ext_add(ct, NF_CT_EXT_NAT)
nf_nat_set_ct(ct, manip)
```

---

## 与 NAT / 防火墙协同

- **首包 NEW** — 可能 **add NAT ext**  upon **SNAT/DNAT rule hit**
- **gc** — 扩展随 `nf_conn` **一并释放**
- **nft** — **同样** 基于 nf_tables + conntrack 扩展

---

## HFT / 运维

| sysctl/调优 | 说明 |
|-------------|------|
| **`nf_conntrack_max`** | 上限 |
| **`nf_conntrack_tcp_timeout_established`** | **ESTABLISHED** 保持时间 |
| **无 NAT 时** | `raw NOTRACK` **跳过 ct** — **减开销**（慎用，丢状态防火墙） |

---

## 本章小结

| 节 | 带走 |
|----|------|
| §1 | **iptables/nft/IPVS/ipset** 栈 |
| §2 | **5 hooks、priority、verdict** |
| §3 | **tuple、nf_conn、helper/RELATED** |
| §4 | **xt_table、链与包路径** |
| §5 | **SNAT/DNAT、ct 先于 NAT** |
| §6 | **按需 ct extension** |

---

## 相关章节

- 下一章：[Ch 10 IPsec](../../chapter-10-ipsec/)
- IPsec 与 NAT 交互：部分 ESP **需 passthrough** 或 **policy 顺序**

---

← [5. NAT](./section-5-网络地址转换-NAT.md) · [Ch 9](../README.md)
