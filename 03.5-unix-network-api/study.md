# UNP HFT 优先读序

> 与 [README.md](./README.md) 互补：README 是书目定位/源码组织，本文是 **HFT 路线视角的读序与取舍**。
>
> 适用对象：**做低延迟交易系统/行情接收**的人。读这份 UNP 不是为了当 API 手册，是为了**搞懂"为什么需要多路复用"、"并发模型四种形态的边界"、"UDP/组播的特殊性"**——这些是 epoll/io_uring 的动机根源。

## 四阶段结构速览

```
1_BasicFoundation  Ch 1-8    简介/TCP-UDP-SCTP/socket入门/基本TCP/CS示例/IO多路/sockopt/基本UDP
2_AdvancedSkill    Ch 11-26  名址转换/守护进程/高级IO/非阻塞/线程
3_DeepMaster       Ch 17-29  ioctl/广播/组播/高级UDP/OOB/信号驱动/raw/链路层
4_ArchitectureDesign Ch 9-31 SCTP/IPv4-6互操作/Unix域/路由/密钥/SCTP高级/IP选项/C-S架构/流
```

## HFT 优先级矩阵

| 优先级 | 章节 | 为什么 |
|--------|------|--------|
| **必精读** | Ch6 select/poll · Ch14 高级IO · Ch16 非阻塞 | reactor 模式地基。select/poll HFT 实际不用，但 Stevens 把"为什么需要多路复用"讲透了——**理解动机比 API 本身重要** |
| **必精读** | Ch5 C/S 示例 · Ch26 线程 | 并发模型四种形态（迭代/多进程/多线程/预派生）的源头，HFT 选哪种的决策依据 |
| **必精读** | Ch8 UDP · Ch21 组播 | **HFT 行情绝大多数走 UDP/组播**，不读这俩等于缺一条腿 |
| **应读** | Ch11 名址转换 · Ch7 sockopt | `getaddrinfo` 双栈、`SO_REUSEADDR`/`SO_KEEPALIVE`/`SO_LINGER` 是 HFT 直接调优点 |
| **跳/略读** | Ch9/10/23 SCTP · Ch28 raw · Ch29 链路层 | SCTP 在 HFT 几乎没人用；raw/链路层属安全/抓包领域，DPDK 路线绕过 |

## 推荐读序（精简版，HFT 路线）

1. **Ch1（1.2 / 1.5 必读，其他略）** — 最小 C/S 闘环，建立全书骨架
2. **Ch2 全章** — TCP/UDP/SCTP 协议层；TIME_WAIT、端口复用是后面所有章节的协议基础
3. **Ch3-4** — socket 基本四 API（`socket/connect/bind/listen/accept/close`），速过
4. **Ch5** — **重读**，C/S 示例四种并发形态的对照
5. **Ch6** — **重读**，select/poll 的**动机和语义**（不学 API 学思想）
6. **Ch7 sockopt** — 挑 `SO_REUSEADDR` / `SO_KEEPALIVE` / `SO_LINGER` 精读，其余备查
7. **Ch8 UDP** — **重读**，UDP 收发的边界条件（消息边界、`recvfrom`/`sendto` 语义）
8. 跳到 [`03-linux-userspace-api`](../03-linux-userspace-api/) 学 epoll（见下方边界说明）
9. 回 UNP 读 **Ch14 高级 IO + Ch16 非阻塞** — 此时已有 epoll 背景，能看懂为什么要非阻塞
10. **Ch21 组播** — HFT 行情订阅核心，必须精读
11. **Ch26 线程** — 多线程服务器模型，重点看线程模型对比，API 略
12. 其他章节按需查

## UNP 的边界（这个 repo 的盲区）

UNP 第3版出版于 2003 年，**只覆盖 select/poll/pselect**。HFT 真正依赖的几项不在本书里，需要去 `03-linux-userspace-api` 补：

- **epoll**（LT/ET、`EPOLLET`、`epoll_wait` 批量返回）— UNP Ch6 只铺垫动机
- **`io_uring`**（Linux 5.1+，HFT 新热）— Stevens 年代没有
- **`SO_REUSEPORT` 多进程同端口**（Linux 3.9+，HFT RSS 关键）— UNP Ch7 的 `SO_REUSEADDR` 只是前身
- **`TCP_QUICKACK` / `TCP_NODELAY` / `TCP_DEFER_ACCEPT`** — HFT 调优必备，UNP Ch7 没几个

> 正确路线：UNP Ch6 学"为什么需要多路复用"和"select/poll 的语义" → 跳到 `03-linux-userspace-api` 学 epoll/uring 的现代实现。
> **UNP 不应承担"实战 API 手册"的角色**，它是动机与语义教科书。

## 章节目录索引

### 1_BasicFoundation
- [Ch01 Introduction](./1_BasicFoundation/Chapter01_Introduction/study.md) — Daytime 最小 C/S 闭环
- [Ch02 TCP/UDP/SCTP](./1_BasicFoundation/Chapter02_TCP_UDP_SCTP/study.md) — 协议层基础
- [Ch03 Socket 程序入门](./1_BasicFoundation/Chapter03_SocketProgramIntro/) — socket 五步法
- [Ch04 基本 TCP Socket](./1_BasicFoundation/Chapter04_BasicTCPSocket/)
- [Ch05 TCP C/S 示例](./1_BasicFoundation/Chapter05_TCP_Client_Server_Demo/) — **四种并发形态**
- [Ch06 I/O 多路 Select/Poll](./1_BasicFoundation/Chapter06_IO_Select_Poll/study.md) — **HFT 必精**
- [Ch07 Socket 选项](./1_BasicFoundation/Chapter07_SocketOption/)
- [Ch08 基本 UDP Socket](./1_BasicFoundation/Chapter08_BasicUDPSocket/) — **HFT 必精**

### 2_AdvancedSkill
- [Ch11 名址转换](./2_AdvancedSkill/Chapter11_Name_Address_Convert/) — `getaddrinfo` 双栈
- [Ch13 守护进程/inetd](./2_AdvancedSkill/Chapter13_Daemon_Inetd/)
- [Ch14 高级 I/O](./2_AdvancedSkill/Chapter14_AdvancedIO_Func/study.md) — **HFT 必精**
- [Ch16 非阻塞 I/O](./2_AdvancedSkill/Chapter16_NonBlockingIO/study.md) — **HFT 必精**
- [Ch26 线程](./2_AdvancedSkill/Chapter26_Thread/) — **并发模型对比**

### 3_DeepMaster
- [Ch17 ioctl](./3_DeepMaster/Chapter17_Ioctl_Operate/)
- [Ch20 广播](./3_DeepMaster/Chapter20_Broadcast/)
- [Ch21 组播](./3_DeepMaster/Chapter21_Multicast/) — **HFT 行情订阅核心**
- [Ch22 高级 UDP](./3_DeepMaster/Chapter22_AdvancedUDPSocket/)
- [Ch24 带外数据 OOB](./3_DeepMaster/Chapter24_OutOfBandData/)
- [Ch25 信号驱动 I/O](./3_DeepMaster/Chapter25_SignalDriveIO/)
- [Ch28 Raw Socket](./3_DeepMaster/Chapter28_RawSocket/) — 略
- [Ch29 数据链路访问](./3_DeepMaster/Chapter29_DataLinkAccess/) — 略

### 4_ArchitectureDesign
- [Ch09/10/23 SCTP](./4_ArchitectureDesign/Chapter09_BasicSCTPSocket/) — 略
- [Ch12 IPv4/IPv6 互操作](./4_ArchitectureDesign/Chapter12_IPv4_IPv6_Interop/)
- [Ch15 Unix 域协议](./4_ArchitectureDesign/Chapter15_UnixDomainProtocol/)
- [Ch30 C/S 设计模式](./4_ArchitectureDesign/Chapter30_Client_Server_DesignMode/) — **架构选型**
- [Ch31 流](./4_ArchitectureDesign/Chapter31_Stream/)
