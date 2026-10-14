# M2 · C++ 网络编程（muduo / PNP）

**模块 `04-cpp/M2`** · 编号即读序 · [返回 C++ 总览](../README.md) · [锁定路线](../../README.md)

> **定位：** C++ 网络服务实验骨架（粘包、epoll、组播等）— 为内核网络 / DPDK / HFT 打底。  
> **前置：** [04 C++（至少 M1 Modern）](../README.md) · [03.5 UNP（C 侧 socket）](../../03.5-unix-network-api/)  
> **书目：** 陈硕 *Linux 多线程服务端编程*

**实验笔记来源：** 早期 Windows 权威仓库的 **复制**（`NN_主题.md`，目录名已改 ASCII）；
当前权威版本即本仓——Linux 环境下直接在本模块演进，不再回同步。

## 实验笔记（13 个，扁平结构）

### A · Stevens/UNP 实验面（1–9）

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

### B · muduo 三条主线（10–13）

> 补于 2026-10-14：1–9 是 UNP 的实验笔记，此前**没有任何一篇落到 muduo 的核心设计**，
> 而 muduo 真正值钱的是下面这四篇。每篇都对照 `github.com/chenshuo/muduo` master 分支源码核过。

| 笔记 | 主题 | 原书对照 |
|------|------|----------|
| [`10_MuduoBuffer.md`](./10_MuduoBuffer.md) | `readv` + 64 KB 栈缓冲、三索引布局、`makeSpace` | 本书第 7 章 |
| [`11_EventLoopThreading.md`](./11_EventLoopThreading.md) | one loop per thread、`eventfd` 唤醒、`runInLoop`/`queueInLoop`、`swap` 缩临界区 | 本书第 2、4 章 |
| [`12_TcpConnectionLifetime.md`](./12_TcpConnectionLifetime.md) | `tie`/`shared_from_this`、`handleClose` 保命 fd、跨线程销毁 | 本书第 1、8 章 |
| [`13_AsyncLogging.md`](./13_AsyncLogging.md) | 四 buffer 异步日志、前台 memcpy / 后台 `swap` 写盘、丢弃策略 | 本书第 5 章 |

## 其他文件

| 路径 | 说明 |
|------|------|
| `PNP-OUTLINE.md` / `PNP-study.md` | 大纲与进度镜像 |
| `PNP-UPSTREAM-README.md` | 上游 README（结构描述以上游仓库为准） |

## 和 03.5 / 12 的分工

| 模块 | 作用 |
|------|------|
| [**03.5 UNP**](../../03.5-unix-network-api/) | Stevens Socket API + **完整 unpv13e 源码树**（C） |
| **M2 PNP（本模块）** | 动手实验笔记（muduo，C++） |
| [**11 TCP/IP**](../../11-tcpip-protocols/) | 协议笔记（抓包/语义） |

## 交叉阅读

- [03.5-unix-network-api](../../03.5-unix-network-api/) · [11-tcpip-protocols](../../11-tcpip-protocols/) · [CSAPP Ch11](../../02-computer-systems/chapter-11-network-programming/)

**上一模块：** [03.5 UNP（C 侧）](../../03.5-unix-network-api/) · 本模块属于 [04 C++](../README.md)  
**下一阶段：** [05 Linux 内核](../../05-linux-kernel/) / [12 TCP/IP 协议](../../11-tcpip-protocols/)
