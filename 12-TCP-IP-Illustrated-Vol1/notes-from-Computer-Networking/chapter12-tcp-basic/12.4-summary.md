# 12.4 总结

> 章级精读：[../study.md#ch12-exam](../study.md#ch12-exam)

## 本节核心目标

收束 TCP 如何在不可靠 IP 上虚拟出**可靠管道**。

---

## 精髓

- 软件算法：**序号、ACK、滑动窗口、动态 RTO** + **rwnd/cwnd**。
- 对应用呈现**字节流**；消息边界由**应用协议**定义。

---

## 下一章

- [ch13 连接管理](../../chapter13-tcp-connection-manage/study.md) — 三次握手/四次挥手、状态机
