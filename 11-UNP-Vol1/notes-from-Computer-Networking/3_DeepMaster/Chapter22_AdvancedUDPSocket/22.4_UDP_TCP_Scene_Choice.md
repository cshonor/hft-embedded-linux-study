# 22.4 何时用 UDP 代替 TCP

> [Ch 2 TCP/UDP](../../1_BasicFoundation/Chapter02_TCP_UDP_SCTP/study.md) · [Ch 20–21](../Chapter20_Broadcast/study.md)

---

## 选型黄金法则

### 必须用 UDP

- **广播、多播** — TCP 仅单播

### 可以使用 UDP

| 场景 | 理由 |
|------|------|
| **简单请求-应答** | DNS、NTP：UDP 约 2 包 vs TCP 握手+挥手 ≥7 包 |
| **可丢包、低延迟** | VoIP、实时游戏 |

### 坚决不用 UDP

- **海量可靠传输**（文件等）  
- 应用层须重做：序号、ACK、窗口、**拥塞控制**  
- 应用层难感知物理拥塞 → 易 **拥塞崩溃**

---

## 个人学习总结

（待填）
