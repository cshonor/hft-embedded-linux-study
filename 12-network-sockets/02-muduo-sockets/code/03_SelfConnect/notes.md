# 03 · 自连接（Self-Connect）

<a id="pnp-03-goal"></a>

## 目标

同一主机上客户端连本机监听端口时，可能出现 **自连接**；分清 `listenfd` 与 `connfd`，`accept` 返回谁。

<a id="pnp-03-unp"></a>

## UNP 对照

- [1.5 listen 队列](../../UNP_Vol1/1_BasicFoundation/Chapter01_Introduction/1.5_Appendix_listen队列.md)
- Ch4 `accept` / `connect`

<a id="pnp-03-pitfalls"></a>

## 坑点

- 关闭错 fd 导致监听套接字被关
- 自连接时 `getpeername` / `getsockname` 表现反常（实验验证）
