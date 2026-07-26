# 14.8 包失序与包重复

> 章级精读：[../study.md#ch14-8](../study.md#ch14-8)

## 本节核心目标

区分 **失序 / 重复** 与真丢包，理解 dup ACK 阈值副作用。

---

## 失序 (Reordering)

| 原因 | TCP 反应 |
|------|----------|
| IP 多路径（ECMP）等 | **dup ACK**；常未真丢包 |
| 乱序跨度大 | 可能误触发 **快速重传**（通常 >3 段易误判） |

---

## 重复 (Duplication)

- 链路层重传等 → 同 SEQ 两份
- 内核 **SEQ 去重**；可配合 **DSACK** 优化发送端

---

## 排障

- Wireshark：`tcp.analysis.duplicate_ack`、`out_of_order`
