# 02 · TCP 字节流与粘包

<a id="pnp-02-goal"></a>

## 目标

理解 **TCP 无消息边界**：一次 `send` 不等于一次 `recv`；应用层须定界（长度前缀、分隔符、固定长）。

<a id="pnp-02-unp"></a>

## UNP 对照

- [1.2 API·read](../../UNP_Vol1/1_BasicFoundation/Chapter01_Introduction/1.2_Appendix_API精读.md)
- Ch3 `readn` / `writen`（本仓库 Ch3 节笔记）

<a id="pnp-02-pitfalls"></a>

## 坑点

- 以为「发 100 字节会一次收满 100」
- 缓冲合并：Nagle、接收窗口、应用读太慢
- 解决：**协议设计**，不是指望 TCP 帮你分包
