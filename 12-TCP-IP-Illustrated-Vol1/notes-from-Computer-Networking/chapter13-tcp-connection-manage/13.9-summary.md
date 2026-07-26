# 13.9 总结

> 章级精读：[../study.md#ch13-exam](../study.md#ch13-exam)

## 本节核心目标

一页收束连接管理面试/排障要点。

---

## 必背

- **三次握手 / 四次挥手** 每步标志与序号含义
- **TIME_WAIT** 与 **2MSL**、短连接端口耗尽
- **SYN 队列 vs Accept 队列**
- **RST vs FIN**
- **MSS / WSCALE / Timestamp** 在 SYN 中协商

---

## Go / Rust 实战映射

| 坑 | 对策 |
|----|------|
| 短连接 → **TIME_WAIT** 占满临时端口 | 复用 `http.Client` Transport / `reqwest` Keep-Alive |
| 服务重启 `Address already in use` | `SO_REUSEADDR`（`ListenConfig` / `socket2`） |
| 对端断线无 RST → Goroutine/Task **永久阻塞** | 所有 Read/Write 加 **Timeout** |

---

## 下一章

- [ch14 超时与重传](../../chapter14-tcp-timeout-retransmit/study.md)
