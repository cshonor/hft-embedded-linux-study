# 8.16 小结

> [study.md](../study.md) · 阶段一收束：Ch 1～8

---

## 章节核心提炼

### 1. 极简 vs 极难

- API：`recvfrom`/`sendto` 即可，无握手挥手  
- 须直面：**丢失、乱序、截断、无流控、异步 ICMP**

### 2. 异步错误

**未 connect UDP** 对 ICMP 报错常**免疫** → 易死锁在 `recvfrom`。

### 3. 高阶：UDP connect

单播优先 **connect**：

- 简化 **`read`/`write`**  
- 过滤非法源（8.8）  
- **传递异步错误**（8.9）

### 4. 工程清单

| 必做 | 手段 |
|------|------|
| 超时 | select / SO_RCVTIMEO / alarm |
| 验源 | 比对地址或 connect |
| 可靠 | 应用层 ACK 重传 |
| 高负载 | 注意 RCVBUF、应用限流 |

---

## TCP vs UDP 编程对照（阶段一）

| | TCP | UDP |
|--|-----|-----|
| 服务器 | listen+accept(+fork) | bind+recvfrom 循环 |
| 客户 | connect | sendto 或 **connect** |
| 边界 | 字节流+readn | **整报文** |
| 关闭 | FIN/RST | 无连接；0 字节≠EOF |

---

> 💡 **后续拓展留白**  
> - 阶段二 Ch9 SCTP  

---

## 个人学习总结

（待填）
