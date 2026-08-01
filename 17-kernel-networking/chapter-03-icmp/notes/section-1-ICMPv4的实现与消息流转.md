# Ch 3 §1 ICMPv4 的实现与消息流转

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 1. ICMPv4 的实现与消息流转

ICMPv4 在 Linux 中挂接在 **IPv4** 之上（protocol number **1**）。栈在 **L3 异常** 或 **收到 ICMP 包** 时进入 ICMP 子系统。

源码主线：`net/ipv4/icmp.c` · 头 `include/uapi/linux/icmphdr.h`

---

## 消息分类

| 类别 | Type 范围（常见） | 用途 |
|------|-------------------|------|
| **差错 (Error)** | 3 Destination Unreachable、11 Time Exceeded、12 Parameter Problem… | 报告 **转发/交付失败** |
| **查询 (Informational)** | 8 Echo Request / 0 Echo Reply、13 Timestamp… | **诊断** — `ping`、`traceroute` |

**差错消息规则（RFC）：** 不对 **差错报差错**、不对 **广播/组播** 首包随意回差错等 — 防 **ICMP 风暴**。

---

## `icmphdr` 结构

```c
struct icmphdr {
    __u8  type;
    __u8  code;
    __sum16 checksum;
    union {
        struct { __be16 id; __be16 sequence; } echo;
        __be32  gateway;
        struct { __be16 __unused; __be16 mtu; } frag;
        /* … 依 type 而变 */
    } un;
};
```

| 字段 | 说明 |
|------|------|
| **type / code** | 消息大类 / 细码（如 3.1 host unreachable、3.3 port unreachable） |
| **checksum** | 覆盖 ICMP 头 + payload |
| **32 位可变部分** | Echo **id/seq**、Fragmentation Needed 的 **MTU** 等 |

---

## 初始化

启动阶段：

```
icmp_init()
  → icmp_sk_init()   /* 每 CPU 一个内核 ICMP raw sock */
```

**每 CPU 独立 ICMP 套接字** — 内核 **自发 ICMP**（差错回复）不必与用户 ping 抢全局锁；与 **per-CPU 栈优化** 一致。

---

## 接收路径（示例 handler）

| 消息 | 典型处理函数 | 行为 |
|------|-------------|------|
| **Echo Request (ping)** | `icmp_echo()` | 交换 src/dst，type→Echo Reply，回送 |
| **Destination Unreachable** | `icmp_unreach()` | 通知上层 socket / 更新 PMTU 发现 |
| **Time Exceeded** | `icmp_discard()` / TTL 处理 | traceroute 依赖 |

入口：`icmp_rcv()` → 按 **type** 分派 **icmp_handlers[]**。

---

## 发送路径 `icmp_send()`

当 **IPv4 栈内部** 遇到异常时调用 — **不是** 应用直接写 ICMP：

| 触发场景 | 典型 ICMP |
|----------|-----------|
| 无路由 / 管理禁止 | 3 Destination Unreachable |
| TCP/UDP 端口无监听 | 3.3 Port Unreachable |
| DF 置位且需分片 | 3.4 Fragmentation Needed (**PMTU**) |
| TTL 耗尽 | 11 Time Exceeded |

**速率限制 (Rate limiting)：** `icmp_xmit()` 路径有 **全局/每目标** 限速 — 防 **DoS 反射** 与 **ICMP 风暴**（恶意扫描触发海量不可达回复）。

```
L3 异常 → icmp_send(type, code, info)
         → 构造 icmphdr + 原 IP 头前 8 字节
         → 经 per-CPU icmp_sk 发出
```

---

## HFT 要点

| 现象 | ICMP 关联 |
|------|-----------|
| **TCP 建连慢/失败** | 中间防火墙 **静默 drop** vs **ICMP unreachable** — 行为不同 |
| **PMTU 黑洞** | DF + 无 ICMP Fragmentation Needed → **挂死** — 共置网要 **固定 MTU / 开 PMTUD 路径** |
| **安全扫描** | 对交易 VLAN **限制入站 ICMP** — 但别误伤 **必要 PMTU** |

→ IPv4 主体：[Ch 4](../../chapter-04-ipv4/)

---

← [Ch 3](../README.md) · 下一节 [2. ICMPv6](./section-2-ICMPv6的扩展与变化.md)
