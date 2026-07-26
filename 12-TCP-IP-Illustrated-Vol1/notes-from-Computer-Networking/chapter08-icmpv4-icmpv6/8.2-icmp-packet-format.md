# 8.2 ICMP 报文

> 章级精读：[../study.md#ch08-2](../study.md#ch08-2)

## 本节核心目标

掌握 ICMP **封装位置** 与统一 **首部前 4 字节**。

---

## 封装

```text
以太网帧 → IP 数据报（协议号 ICMP）→ ICMP 报文 → [可选] 触发差错的原 IP 首部+8字节
```

- ICMP 载荷在 **IP 数据报的数据区**，非独立三层 PDU 名以外的通道。

---

## 首部（v4 / v6 共性）

| 字段 | 作用 |
|------|------|
| **Type** | 大类（差错 / Echo / ND…） |
| **Code** | 子类（如不可达原因） |
| **Checksum** | 覆盖 ICMP 首部 + 数据 |

- 具体事件 = **Type + Code** 组合查表。

---

## Go/Rust 提示

- 自定义 ping 需 **Raw Socket**（`IPPROTO_ICMP` / `IPPROTO_ICMPV6`），通常要**管理员权限**。
