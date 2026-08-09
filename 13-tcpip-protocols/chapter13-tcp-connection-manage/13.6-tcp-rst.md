# 13.6 重置报文段 (RST)

> 章级精读：[../study.md#ch13-6](../study.md#ch13-6)

## 本节核心目标

理解 **RST** 触发场景与 `Connection reset by peer`。

---

## 典型场景

| 场景 | 行为 |
|------|------|
| 连接**不存在**的端口 | 回 **RST**（非 SYN-ACK） |
| 异常终止（`SO_LINGER`、进程崩溃） | 发 **RST** 而非 FIN 四次挥手 |
| 半开连接检测到对端已死 | 可能 RST |
| 非法 Seq 等 | RST |

---

## 与 FIN 区别

- **FIN**：优雅关闭，仍可走状态机。
- **RST**：立即拆除 TCB；对端 read/write → **ECONNRESET**。

---

## 开发

- Go：`use of closed network connection` / `connection reset`
- 务必 **Read/Write 超时**，防半开挂死 → 与 Keepalive 配合 [ch17](../../chapter17-tcp-keepalive/study.md)
