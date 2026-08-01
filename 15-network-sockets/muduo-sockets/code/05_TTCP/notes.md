# 05 · TTCP（吞吐测试）

<a id="pnp-05-goal"></a>

## 目标

测量 TCP 吞吐；批量 `read`/`write`、禁用 Nagle、窗口与缓冲调优。

<a id="pnp-05-unp"></a>

## UNP 对照

- Ch3.9 `readn`/`writen`、Ch14 高级 I/O

<a id="pnp-05-pitfalls"></a>

## 坑点

- 小包 + Nagle → 吞吐假象
- 用户态缓冲太小导致系统调用过多
