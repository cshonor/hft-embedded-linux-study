# 第3章 订单簿深度与行情解析

> **Book Builder · O(1) 更新 · Feed 解析**

← [chapter-08 Book Builder](./chapter-08-超低延迟核心引擎开发.md#2-book-builder) · [chapter-01 §1](./chapter-01-高频交易基础与生态.md#1-系统核心架构关键路径)

---

## 1. Book Builder 职责

- 消费 **Gateway IN** 解码后的事件
- 维护 **price level**（价 → 量 → 订单链表，视 feed 粒度）
- 暴露 **BBO / L2** 给 Strategy（**无锁读** 或 **版本号快照**）

---

## 2. 性能目标

| 指标 | 目标 |
|------|------|
| **单次 update** | 接近 **O(1)** |
| **内存** | **预分配** level pool |
| **热路径** | 无锁、无分配、无字符串 |

---

## 3. 行情解析

| 步骤 | 说明 |
|------|------|
| **Wire → struct** | 二进制 **fixed layout** |
| **序列 gap** | 触发 **snapshot 重同步** |
| **Normalize** | 多交易所 **统一内部 Event 类型** |

→ [chapter-06 协议](./chapter-06-低延迟网络与协议优化.md)
