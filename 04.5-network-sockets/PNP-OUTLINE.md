# PNP 实验大纲（常见模块）

> 集号因课程版本而异；听课时在 [PNP-study.md](./PNP-study.md) 补「第 N 集 → 实验目录」。

| 序号 | 目录 | 主题 / 坑点 | UNP 对照 |
|------|------|-------------|----------|
| 01 | [01_SocketBasics](./01_SocketBasics.md) | 地址、字节序、Daytime | Ch1 |
| 02 | [02_TCPByteStream](./02_TCPByteStream.md) | 无消息边界、`read` 循环 | Ch2–3 |
| 03 | [03_SelfConnect](./03_SelfConnect.md) | `listenfd`/`connfd`、自连接 | Ch1.5、Ch4 |
| 04 | [04_Netcat](./04_Netcat.md) | 双向转发、半关闭 | Ch5–6、Ch14 |
| 05 | [05_TTCP](./05_TTCP.md) | 吞吐、`readn`/`writen` | Ch3.9、Ch14 |
| 06 | [06_NonBlockingIO](./06_NonBlockingIO.md) | `EAGAIN`、`EINTR` | Ch16 |
| 07 | [07_IO_epoll](./07_IO_epoll.md) | select/poll/epoll | Ch6、Ch16 |
| 08 | [08_UDP_Multicast](./08_UDP_Multicast.md) | UDP、组播 | Ch8、Ch21–22 |
| 09 | [09_Serialization](./09_Serialization.md) | Protobuf 对齐、版本 | PNP 独有 |

**已实现 Rust**：04 Netcat（最小可用）。其余目录为占位，听课后补 `original_cpp` 与笔记。
