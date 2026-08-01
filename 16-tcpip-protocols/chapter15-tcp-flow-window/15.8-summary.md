# 15.8 总结

> 章级精读：[../study.md#ch15-exam](../study.md#ch15-exam)

## 本节核心目标

收束流控算法矩阵与工程清单。

---

## 机制矩阵

| 机制 | 责任方 | 目标 |
|------|--------|------|
| **Nagle** | 发送方 | 抑制小包 |
| **延迟 ACK** | 接收方 | 少纯 ACK、捎带 |
| **滑动窗口** | 双方 | 端到端流控 |
| **Persist** | 发送方 | 破零窗口死锁 |
| **Clark / SWS** | 接收方 | 防糊涂窗口 |

---

## Go / Rust 避坑：~40ms 毛刺

| 语言 | Nagle 默认 |
|------|------------|
| **Go** | 默认 **NoDelay=true** |
| **Rust** | 默认 **开启** Nagle → 务必 `set_nodelay(true)` |

- 游戏/高频 RPC 卡顿：先查 **Nagle + Delayed ACK**

---

## 下一章

- [ch16 拥塞控制](../../chapter16-tcp-congestion-control/study.md)
