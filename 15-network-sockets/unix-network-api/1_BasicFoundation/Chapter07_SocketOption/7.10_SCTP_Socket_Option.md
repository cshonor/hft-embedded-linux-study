# 7.10 SCTP 套接字选项

> 级别 **`IPPROTO_SCTP`** · SCTP 协议：Ch 2.5 · 套接字 API：Ch 9

---

## 核心主旨

SCTP **多流、多宿** → 选项多且常传**结构体**。

| 选项 | 说明 |
|------|------|
| **SCTP_EVENTS** | 订阅 SCTP **通知**（状态变、地址变、发送失败等） |
| **SCTP_INITMSG** | 配置 **INIT** 默认参数（最大流数、重传次数等） |
| **SCTP_NODELAY** | 类似 TCP，禁用 SCTP 级 **Nagle** |
| **SCTP_PRIMARY_ADDR** | 多宿下设置/更改 **主地址** |
| **SCTP_AUTOCLOSE** | 空闲超时自动关闭关联（一对多套接字有用） |

---

> 💡 **后续拓展留白**  
> - Ch 23 高级 SCTP 选项扩展  

---

## 个人学习总结

（待填）
