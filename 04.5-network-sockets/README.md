# 04.5 · C++ 网络 Socket 编程（muduo / PNP）

**文件夹 04.5** · 编号即读序 · [锁定路线](../README.md)

> **定位：** C++/C 网络服务实验骨架（粘包、epoll、组播等）— 为内核网络 / DPDK / HFT 打底。  
> **前置：** [04 C++](../04-cpp/) · [03.5 UNP](../03.5-unix-network-api/)  
> **书目：** 陈硕 *Linux 多线程服务端编程*

**本地权威仓库：** `C:\Users\12392\Desktop\Computer Networking\PNP`  
本模块从该目录 **复制** 实验笔记（`NN_主题.md`，目录名已改 ASCII）；真正写代码时在权威仓库补全。

## 实验笔记（9 个，扁平结构）

| 笔记 | 主题 | UNP 对照 |
|------|------|----------|
| [`01_SocketBasics.md`](./01_SocketBasics.md) | 地址、字节序、Daytime | Ch1 |
| [`02_TCPByteStream.md`](./02_TCPByteStream.md) | TCP 粘包/半包、`read` 循环 | Ch2–3 |
| [`03_SelfConnect.md`](./03_SelfConnect.md) | `listenfd`/`connfd`、自连接 | Ch1.5、Ch4 |
| [`04_Netcat.md`](./04_Netcat.md) | 双向转发、半关闭 | Ch5–6、Ch14 |
| [`05_TTCP.md`](./05_TTCP.md) | 吞吐、`readn`/`writen` | Ch3.9、Ch14 |
| [`06_NonBlockingIO.md`](./06_NonBlockingIO.md) | `EAGAIN`、`EINTR` | Ch16 |
| [`07_IO_epoll.md`](./07_IO_epoll.md) | select/poll/epoll | Ch6、Ch16 |
| [`08_UDP_Multicast.md`](./08_UDP_Multicast.md) | UDP、组播 | Ch8、Ch21–22 |
| [`09_Serialization.md`](./09_Serialization.md) | Protobuf 对齐、版本 | PNP 独有 |

## 其他文件

| 路径 | 说明 |
|------|------|
| `PNP-OUTLINE.md` / `PNP-study.md` | 大纲与进度镜像 |
| `PNP-UPSTREAM-README.md` | 上游 README（结构描述以上游仓库为准） |

## 和 03.5 / 12 的分工

| 模块 | 作用 |
|------|------|
| [**03.5 UNP**](../03.5-unix-network-api/) | Stevens Socket API + **完整 unpv13e 源码树**（C） |
| **04.5 PNP** | 动手实验笔记（muduo，C++） |
| [**12 TCP/IP**](../12-tcpip-protocols/) | 协议笔记（抓包/语义） |

## 交叉阅读

- [03.5-unix-network-api](../03.5-unix-network-api/) · [12-tcpip-protocols](../12-tcpip-protocols/) · [CSAPP Ch11](../02-computer-systems/chapter-11-network-programming/)

**上一章：** [04 C++](../04-cpp/)  
**下一章：** [05 Linux 内核](../05-linux-kernel/)
