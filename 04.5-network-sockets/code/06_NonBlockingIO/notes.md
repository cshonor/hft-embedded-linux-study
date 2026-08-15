# 06 · 非阻塞 I/O

<a id="pnp-06-goal"></a>

## 目标

`fcntl` `O_NONBLOCK`；`EAGAIN`/`EWOULDBLOCK` 不是致命错误；与阻塞模型对比。

<a id="pnp-06-unp"></a>

## UNP 对照

- [1.4 特殊 errno](../../UNP_Vol1/1_BasicFoundation/Chapter01_Introduction/1.4_ErrorHandlingWrapper.md)
- Ch16 Nonblocking I/O

<a id="pnp-06-pitfalls"></a>

## 坑点

- 非阻塞 `connect` 返回 `EINPROGRESS`
- 忙等 vs 事件驱动（见 07）
