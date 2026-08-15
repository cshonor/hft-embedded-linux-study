# 01 · Socket 基础

<a id="pnp-01-goal"></a>

## 目标

字节序、`sockaddr`、`connect`/`bind`；用 Daytime 跑通 TCP 客户端/服务器。

<a id="pnp-01-unp"></a>

## UNP 对照

| 内容 | 链接 |
|------|------|
| 客户端 | [UNP 1.2](../../UNP_Vol1/1_BasicFoundation/Chapter01_Introduction/1.2_SimpleTimeClient.md) |
| 服务器 | [UNP 1.5](../../UNP_Vol1/1_BasicFoundation/Chapter01_Introduction/1.5_SimpleTimeServer.md) |
| C/S 联合 | [1.12 附录](../../UNP_Vol1/1_BasicFoundation/Chapter01_Introduction/1.12_Appendix_DaytimeCS联合流程.md) |
| Rust 已有 | [Ch1 code](../../UNP_Vol1/1_BasicFoundation/Chapter01_Introduction/code/README.md) |

<a id="pnp-01-pitfalls"></a>

## 坑点（PNP 常强调）

- `sin_addr` = IP，`sin_port` = 端口（`htons`）
- 特权端口 13：非 root 可改 10013 做实验
- `read` 须循环直到 FIN（见 02 粘包）
