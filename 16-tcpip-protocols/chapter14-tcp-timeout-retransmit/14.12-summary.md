# 14.12 总结

> 章级精读：[../study.md#ch14-exam](../study.md#ch14-exam)

## 本节核心目标

收束「等多久 / 怎么补 / 误报」三板斧。

---

## 必背

| 支柱 | 内容 |
|------|------|
| **等多久** | SRTT + 4×RTTVAR；Karn；RTTM |
| **怎么补** | Timer → **3 dup ACK** → **SACK 精准补洞** |
| **误报** | DSACK / Eifel / F-RTO 撤销误降 cwnd |

---

## Go / Rust 实战

| 坑 | 对策 |
|----|------|
| 拔网线后 `Write` **卡 13–15 分钟** | Linux `tcp_retries2`≈15 次指数退避 |
| Goroutine/Task 被挂死 | 应用 **Deadline** + 底层 **`TCP_USER_TIMEOUT`**（如 10s） |
| **队头阻塞** | 丢 1 包则应用读阻塞，即使 SACK 已收后续段 → **QUIC/HTTP3** 动机 |

---

## 下一章

- [ch15 数据流与窗口](../../chapter15-tcp-flow-window/study.md) — rwnd、Nagle
