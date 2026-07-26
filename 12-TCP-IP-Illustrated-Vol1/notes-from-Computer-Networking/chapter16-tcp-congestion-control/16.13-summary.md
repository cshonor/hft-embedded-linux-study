# 16.13 总结

> 章级精读：[../study.md#ch16-exam](../study.md#ch16-exam)

## 本节核心目标

收束拥塞控制考点与系统调优认知。

---

## 必背

| 维度 | 要点 |
|------|------|
| 公式 | **W = min(cwnd, awnd)** |
| 经典 | 慢启动、拥塞避免、AIMD、Tahoe vs Reno |
| 改进 | NewReno、SACK、Limited Transmit |
| 高速 | **CUBIC**；**BBR**（sysctl 切换） |
| 网络侧 | Bufferbloat、RED、**ECN** |

---

## Go / Rust 实战

- **应用层无慢启动 API** — 算法在内核
- 跨国吞吐上不去：先查 **`tcp_congestion_control`**、socket 缓冲、**WSCALE**
- 弱网可试：`sysctl net.ipv4.tcp_congestion_control=bbr`

---

## 下一章

- [ch17 Keepalive](../../chapter17-tcp-keepalive/study.md)
