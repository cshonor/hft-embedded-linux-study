# 09 · 序列化陷阱（Protobuf 等）

<a id="pnp-09-goal"></a>

## 目标

跨语言、跨版本、内存对齐；**UNP 少讲**，PNP 工程向重点。

<a id="pnp-09-unp"></a>

## UNP 对照

- 无直接章节；网络字节序见 Ch3、`htonl`

<a id="pnp-09-pitfalls"></a>

## 坑点

- 结构体 `memcpy` 上线（对齐、32/64 位）
- protobuf `optional` / `packed` 与旧客户端兼容
