# 11.2 TCP 流控制

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md)

**核心主旨**：滑动窗口按接收缓冲调节速率；**零窗口** = 接收方「装不下」→ 卡顿数十秒。

## 核心知识点

### 11.2.1 调整窗口大小

| 项 | 说明 |
|----|------|
| **Receive Window** | TCP 头字段，告知发送方**还可发送多少字节**未确认数据 |
| 动态 | 接收方处理慢 → 后续 ACK 中 **Window 减小** → 发送方降速 |
| 恢复 | 缓冲释放 → Window 增大 |

Wireshark：展开 TCP 头 **Window size**；启用 **Window size scaling** 因子（选项）。

---

### 11.2.2 零窗口与恢复

| 项 | 说明 |
|----|------|
| **Zero Window** | Window = **0** → 发送方**停止发数据** |
| **Keep-Alive** | 发送方周期性探针（非应用心跳）询问是否恢复 |
| **Window Update** | 接收方 Window 变非零 → 恢复传输 |

**过滤器**：`tcp.analysis.zero_window` · `tcp.analysis.window_update` · `tcp.analysis.keep_alive`

---

### 11.2.3 滑动窗口实战表征

| 时序 | 特征 |
|------|------|
| 恶化前 | 多个 ACK 中 **Win 急剧递减** |
| 触发 | `[TCP ZeroWindow]` |
| 之后 | `[TCP Keep-Alive]` 间隔常**成倍增加**（如 3.4s → 6.8s → 13.5s） |
| 用户感知 | **长时间「卡住」**（打嗝） |

**TCP Stream Graph** → **Window Scaling**：对照吞吐塌陷与 Zero Window 时刻。

与 [§10.4 打印机](../chapter-10-basic-scenario/04-printer-fault.md) 案例一致：接收端设备/服务瓶颈。

## 抓包/实操记录

| 任务 | 操作 |
|------|------|
| 列零窗口 | `tcp.analysis.zero_window` |
| 看谁发的 | Zero Window 包 **源 IP** = 背压方（常为服务器） |
| IO Graph | bytes/s 在 Zero Window 后近 0 |

## 疑问与总结

- 零窗口 **≠ 路由器拥塞**；优先查**接收主机 CPU/内存/应用读 socket 慢**。
- 发送方也有窗口（拥塞窗口），本章侧重 **接收窗口** 导致的停流。
