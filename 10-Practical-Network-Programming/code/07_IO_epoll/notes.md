# 07 · I/O 复用（select / poll / epoll）

<a id="pnp-07-goal"></a>

## 目标

**就绪**通知 vs 真正 I/O；水平触发 vs 边缘触发；单线程多连接。

<a id="pnp-07-unp"></a>

## UNP 对照

- Ch6 I/O Multiplexing、Ch16

<a id="pnp-07-pitfalls"></a>

## 坑点

- ET 必须读到 `EAGAIN`
- `listenfd` 与 `connfd` 同处 epoll 的写法
- `select` 的 fd 上限与 `FD_SETSIZE`
