# 13.8 与 TCP 连接管理相关的攻击

> 章级精读：[../study.md#ch13-8](../study.md#ch13-8)

## 本节核心目标

认识 **SYN Flood**、半开与序列号攻击及防护。

---

## SYN Flood

- 伪造源 IP 海量 **SYN** → 占满**半连接队列** → 合法用户无法建连。
- **SYN Cookies**：SYN-ACK 中编码信息，**不分配 TCB** 直至 ACK 有效。

---

## 其他

| 攻击 | 说明 |
|------|------|
| **RST 攻击** | 盲猜 Seq 发 RST 断连（难但理论存在） |
| **连接劫持** | 需猜 Seq/ACK，TLS 缓解 |

---

## 运维

- `syncookies`、限速、防火墙 SYN proxy、WAF。
