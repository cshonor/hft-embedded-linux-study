# 10 PNP · 源码

来源：本机 [`Desktop/Computer Networking/PNP`](file:///C:/Users/12392/Desktop/Computer%20Networking/PNP)（与 [cshonor/Computer-Networking](https://github.com/cshonor/Computer-Networking) 同源）。

目录名已改成 **ASCII**（避免 Windows 下中文目录乱码）。

| 目录 | 主题 |
|------|------|
| `01_SocketBasics` | Socket 基础 |
| `02_TCPByteStream` | TCP 粘包/半包 |
| `03_SelfConnect` | 自连接 |
| `04_Netcat` | Netcat |
| `05_TTCP` | TTCP |
| `06_NonBlockingIO` | 非阻塞 |
| `07_IO_epoll` | epoll |
| `08_UDP_Multicast` | UDP 组播 |
| `09_Serialization` | 序列化陷阱 |

每个实验典型结构：`notes.md` + `original_c/` + `original_cpp/` + 可选 rewrite。  
**现状：** 上游里不少 `original_*` 仍是占位（`.gitkeep`），真正写代码时在对应目录补全即可。

上游说明：[`PNP-UPSTREAM-README.md`](../PNP-UPSTREAM-README.md) · [`PNP-OUTLINE.md`](../PNP-OUTLINE.md) · [`PNP-study.md`](../PNP-study.md)
