# 第 1 章：简介（厚版）

> 阶段一 · 逐节 `1.x_*.md`（含核心主旨、细节、逻辑、易错、留白）

## 小节索引

| 节 | 目录 | 备注 |
|----|------|------|
| 1.1 | [1.1_Overview](./1.1_Overview.md) | C/S、TCP/IP、LAN/WAN、[C/S→B/S](./1.1_Overview.md#ch1-1-cs-bs) |
| 1.2 | [1.2_SimpleTimeClient](./1.2_SimpleTimeClient.md) | **主笔记** + [阅读地图](1.2_SimpleTimeClient.md#ch1-2) |
| 1.2 附录 | [API](1.2_Appendix_API精读.md) · [源码](1.2_Appendix_源码与考点.md) · [C/字节序](1.2_Appendix_C新手与字节序.md) · [Rust](1.2_Appendix_Rust客户端.md) | 详文拆开、不重复 |

<a id="ch1-2"></a>

### 1.2 时间客户端（速记）

→ [主笔记·五步法/IP端口](1.2_SimpleTimeClient.md#ch1-2-flow) · [API connect/read](1.2_Appendix_API精读.md) · [源码](1.2_Appendix_源码与考点.md) · [C FAQ](1.2_Appendix_C新手与字节序.md)

**五步法**：`socket(AF_INET,TCP,0)` → 填 **sin_addr=IP / sin_port=13** → `connect` → **`while read`** → `exit` · [socket 三参数](1.2_SimpleTimeClient.md#ch1-2-socket)

| 易错 | 规范 |
|------|------|
| 搞不清 IP/端口 | **sin_addr / sin_port** 见主笔记表 |
| 一次 read 读全 | **while** |
| connect 失败再 connect | **close + 新 socket** |

**13 = Daytime（RFC865）** → [端口说明](1.2_Appendix_Daytime端口13.md)；对端 [1.5](./1.5_SimpleTimeServer.md)

| 1.3 | [1.3_ProtocolIndependence](./1.3_ProtocolIndependence.md) | **厚** · 协议无关/getaddrinfo |
| 1.4 | [1.4_ErrorHandlingWrapper](./1.4_ErrorHandlingWrapper.md) | **厚** · 包裹/errno/Pthread |
| 1.5 | [1.5_SimpleTimeServer](./1.5_SimpleTimeServer.md) | **厚** · 六步/listenfd·connfd |

<a id="ch1-4"></a>

### 1.4 错误处理 · 包裹函数（速记）

→ 精读：[1.4_ErrorHandlingWrapper.md](./1.4_ErrorHandlingWrapper.md) · [包裹源码](1.4_ErrorHandlingWrapper.md#ch1-4-source)

**规则**：小写 = 系统调用；**大写 = 包裹**（内建 `err_sys`）

| 范式 | 失败时 |
|------|--------|
| Unix API | **-1** + **errno** → `err_sys` |
| **Pthread** | **返回错误码**，**不**置 errno |

**不能盲用大写**：**EINTR / EAGAIN / ECONNRESET** → 小写 + 分支（Ch 5/6）

**链路**：1.2 裸 `if` → 1.4 `Socket`/`Connect` → Ch5/6 改造

| 1.7 | [1.7_OSIModel](./1.7_OSIModel.md) | **厚** · OSI/TCP/IP/Socket 位置 |
| 1.12 | [1.12_Summary](./1.12_Summary.md) | **厚** · 全章总纲/HFT路线/背诵版 |
| 1.6～1.11 | 各目录 | 索引/ BSD / 测试网 / POSIX / LP64 |

<a id="ch1-5"></a>

### 1.5 时间服务器（速记）

→ [1.5 精读](./1.5_SimpleTimeServer.md) · [listen 队列](1.5_Appendix_listen队列.md) · [C/Rust 全文](1.5_Appendix_源码与Rust.md)

**六步**：`Socket` → **Bind(INADDR_ANY:13)** → `Listen` → **`Accept` 循环** → `Write` → **`Close(connfd)`**

| fd | 用途 |
|----|------|
| **listenfd** | 只 accept |
| **connfd** | 只一路 I/O，完即关 |

**迭代** 串行 · 配 [1.2 客户](./1.2_SimpleTimeClient.md)

<a id="ch1-7"></a>

### 1.7 OSI & TCP/IP 分层（速记）

→ [1.7_OSIModel.md](./1.7_OSIModel.md#ch1-7-socket)

**TCP/IP 四层** · 应用层 = OSI 5+6+7 · **Socket = 用户态调内核协议栈**

| 套接字 | 层级 |
|--------|------|
| TCP/UDP 普通 | 只见应用数据，内核加 TCP/IP/以太网头 |
| Raw Ch28 | 可到 IP，自定义传输/ICMP |
| 链路 Ch29 | 二层帧 |

**封装**：应用 → Socket → 内核加 TCP/IP/帧头 · **解封装**：read 前内核已剥头

<a id="ch1-3"></a>

### 1.3 协议无关性（速记）

→ 精读：[1.3_ProtocolIndependence.md](./1.3_ProtocolIndependence.md) · [对照表](1.3_ProtocolIndependence.md#ch1-3-struct) · [三套源码](1.3_ProtocolIndependence.md#ch1-3-source)

**1.2 硬编码 IPv4** → **图 1-6 硬编码 IPv6**（仍耦合）→ **`getaddrinfo` + `AF_UNSPEC`**（双栈）

| 文件 | 模式 |
|------|------|
| [code/1.2/…/daytimetcpcli.c](./code/1.2_SimpleTimeClient/original_c/daytimetcpcli.c) | IPv4 硬编码 |
| [code/1.3/…/daytimetcpcli6.c](./code/1.3_ProtocolIndependence/original_c/daytimetcpcli6.c) | IPv6 硬编码 |
| [code/1.3/…/daytimetcpcligai.c](./code/1.3_ProtocolIndependence/original_c/daytimetcpcligai.c) | 协议无关 |

**规范**：禁用 `gethostbyname`；**`freeaddrinfo`** 必调 → [Ch 11.6](../../2_AdvancedSkill/Chapter11_Name_Address_Convert/11.6_Getaddrinfo_Func.md)

<a id="ch1-12"></a>

### 1.12 本章小结（速记）

→ [1.12_Summary.md](./1.12_Summary.md#ch1-12-cheat) · [HFT 自检清单](./1.12_Summary.md#ch1-12-hft-checklist) · [后续路线](./1.12_Summary.md#ch1-12-roadmap)

**定位**：Daytime TCP **最小 C/S 闭环** = 全书骨架 · [1.2](./1.2_SimpleTimeClient.md) + [1.5](./1.5_SimpleTimeServer.md) · [图1-5/1-9 联合流程](./1.12_Appendix_DaytimeCS联合流程.md)

**四规范**：大写包裹 · `SA` 宏 · htons/pton · 协议无关→Ch11

**HFT 优先**：Ch6/14 epoll · Ch5/26 并发 · Ch11 双栈 · Ch8/21 UDP/组播

## 速记

```text
C/S；Daytime:13；客户 connect+while read；服 bind listen accept write close(connfd)。
大写包裹；errno；[1.12 全章背版](./1.12_Summary.md#ch1-12-cheat)。
Ch6/14 epoll；Ch11 双栈；[1.7](./1.7_OSIModel.md) Socket 分界。
```
