# 第 20 章 · Hash Tables（哈希表） · 开放寻址与线性探测（Open Addressing and Linear Probing）

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-hash-functions-and-load-factor.md](./02-hash-functions-and-load-factor.md)

---

**不用** 分离链接法（bucket + 链表）：

| 开放寻址 | 说明 |
|----------|------|
| 存储 | **单数组 `entries[]`** |
| 冲突 | **线性探测**：`index = (index + 1) % capacity` 找下一空位 |
| 缓存 | 连续内存 · **CPU cache line** 友好 |

**Entry 状态**：

| 状态 | 含义 |
|------|------|
| **空 (NULL key)** | 从未使用 |
| **占用** | 有效键值 |
| **墓碑 (Tombstone)** | 已删 · 查找跳过 · **插入可复用** |

**为何墓碑？**

- 直接清空槽位 → 打断探测链 → **后续键「找不到」**。
- 墓碑标记「此处曾有键，请继续探测」。

---
