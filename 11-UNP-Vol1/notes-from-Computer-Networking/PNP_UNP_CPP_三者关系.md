# 陈硕 PNP · UNP · C++ 课 — 三者关系与学习路线

> **PNP** = 陈硕《Practical Network Programming》（实用网络编程）  
> **UNP** = 本仓库 [UNP_Vol1](./README.md)（Stevens《UNIX Network Programming》卷 1）  
> **C++ 课** = 陈硕另一门偏 **C++ 工程实践** 的课程（语言/STL/架构，非本文件主角）

<a id="pnp-what"></a>

## 一、PNP 是什么？（不是 UNP，也不是那门 C++ 课）

| 项 | 说明 |
|----|------|
| **全称** | 《Practical Network Programming》· 国内常称「陈硕网络编程课」 |
| **实现语言** | 课程用 **C++** 组织工程，但大量示例底层是 **POSIX Socket API（C 接口）** |
| **目标** | **实战驱动**：粘包、自连接、非阻塞 I/O、并发模型、序列化陷阱等「坑」 |
| **与 UNP** | **不是** UNP 官方配套课；是在 UNP 知识点上的 **现代实战扩展 + 坑点拆解** |

```text
UNP（圣经·系统 API 理论）  ←──基础──  PNP（实战·踩坑·实验）
                                      │
C++ 工程课（语言/架构）  ←──交叉在并发/性能──┘
```

---

<a id="pnp-vs-unp"></a>

## 二、PNP 与 UNP 的关系

| 维度 | **UNP 卷 1** | **陈硕 PNP** |
|------|--------------|--------------|
| 形态 | 教科书 + API 全书索引 | 视频/实验 + 工程案例 |
| 语言 | **C**（`unp.h`、包裹函数） | **C++ 外壳 + C Socket** |
| 深度 | 协议/API **系统化** | **坑、边界、线上问题** |
| 实验 | Daytime、echo、fork 服务器 | **TTCP、Netcat、procmon** 等 |
| 定位 | 把知识点 **串成体系** | 把理论 **打成肌肉记忆** |

**一句话**：PNP ≈ **UNP 的现代实战升级版**；先跟 PNP 踩坑，再用 UNP 本仓库 **系统化归档**。

---

<a id="pnp-vs-cpp"></a>

## 三、PNP 与「那门 C++ 课」的区别

| | **PNP（网络编程）** | **陈硕 C++ 工程课** |
|---|---------------------|---------------------|
| 重心 | **网络 I/O、协议、并发模型** | **C++ 特性、STL、架构、内存模型** |
| Socket | 核心 | 仅交叉（并发/性能） |
| 适合 | 后端、网关、HFT、中间件 | 大型 C++ 工程能力 |

两门课 **互补**，不能互相替代。

---

<a id="learn-path"></a>

## 四、怎么学（后端 / HFT 向）

1. **PNP**：用 C/C++ 跑通 Socket，亲手踩 **粘包、自连接、非阻塞 I/O**。  
2. **本仓库**：边课边记 → [UNP Ch1](./1_BasicFoundation/Chapter01_Introduction/study.md) Daytime → Ch4/5/6…  
3. **Rust/Go 重写**：在 [PNP/code/](../PNP/code/README.md) 做 Netcat、TTCP 等实验；Daytime 见 Ch1 [Rust rewrite](./1_BasicFoundation/Chapter01_Introduction/code/README.md)。  
4. **UNP 原书**：查漏补缺，建立完整索引。

---

<a id="pnp-unp-map"></a>

## 五、PNP 主题 ↔ UNP 卷 1 章节对照（按主题，非逐集）

> 陈硕课程 **期数/标题因版本而异**，下表按 **常见 PNP 模块** 对齐本仓库 UNP 路径；你听课时在「集号」列自行补笔记链接。

| PNP 常见主题 / 实验 | 核心「坑」 | UNP 卷 1 对照 | 本仓库入口 |
|---------------------|------------|---------------|------------|
| Socket 入门、Daytime、地址 | 字节序、`sockaddr`、13 端口 | **Ch1** | [1.2 客户](./1_BasicFoundation/Chapter01_Introduction/1.2_SimpleTimeClient.md) · [1.5 服](./1_BasicFoundation/Chapter01_Introduction/1.5_SimpleTimeServer.md) · [C/S 联合流程](./1_BasicFoundation/Chapter01_Introduction/1.12_Appendix_DaytimeCS联合流程.md) |
| 包裹函数、`errno` | 错误处理范式 | **Ch1 §1.4** | [1.4](./1_BasicFoundation/Chapter01_Introduction/1.4_ErrorHandlingWrapper.md) |
| 协议无关、`getaddrinfo` | 双栈 | **Ch1 §1.3 → Ch11** | [1.3](./1_BasicFoundation/Chapter01_Introduction/1.3_ProtocolIndependence.md) |
| TCP 字节流、**粘包** | `read` 循环、无消息边界 | **Ch2 + Ch3** | [1.2 API·read](./1_BasicFoundation/Chapter01_Introduction/1.2_Appendix_API精读.md#ch1-2-read) · [Ch3 readn](../1_BasicFoundation/Chapter03_SocketProgramIntro/) |
| `connect`/`accept`、自连接 | `listenfd`/`connfd` | **Ch1.5、Ch4** | [1.5 listen 队列](./1_BasicFoundation/Chapter01_Introduction/1.5_Appendix_listen队列.md) |
| 迭代 / **fork 并发** 服务器 | 父子进程、`close` 语义 | **Ch4–5** | [Ch4](../1_BasicFoundation/Chapter04_BasicTCPSocket/) · [Ch5](../1_BasicFoundation/Chapter05_TCP_Client_Server_Demo/) |
| **I/O 多路复用** select/poll/epoll | 就绪 vs 阻塞 | **Ch6、Ch16** | [Ch6](../1_BasicFoundation/Chapter06_IO_Select_Poll/) · [Ch16](../../2_AdvancedSkill/Chapter16_NonBlockingIO/) |
| **非阻塞 I/O**、边缘触发 | `EAGAIN`、`EINTR` | **Ch16、Ch14** | [1.4 特殊 errno](./1_BasicFoundation/Chapter01_Introduction/1.4_ErrorHandlingWrapper.md#ch1-4-special) |
| UDP、**组播** | 无连接、丢包 | **Ch8、Ch21–22** | [Ch8](../1_BasicFoundation/Chapter08_BasicUDPSocket/) |
| **TTCP**、吞吐测试 | 批量读写、`writen` | **Ch3.9、Ch14** | readn/writen 节 |
| **Netcat** 类工具 | 双向转发、半关闭 | **Ch5–6、Ch14** | [PNP 04 Netcat](../PNP/code/04_Netcat/notes.md) |
| 原始 / 链路套接字 | 自己封 IP/TCP 头 | **Ch28–29** | [Ch28](../3_DeepMaster/Chapter28_RawSocket/) |
| 序列化、Protobuf 陷阱 | 对齐、版本（**UNP 少讲**） | PNP 独有 | 记实验笔记即可 |
| 线程池、并发模型 | 锁、条件变量 | **Ch26** + PNP | [Ch26](../../2_AdvancedSkill/Chapter26_Thread/) |

### 填写模板（你听课时追加）

```markdown
| 集号 | PNP 标题 | 本节坑点 | UNP 章 | 仓库笔记链接 |
|------|----------|----------|--------|--------------|
| 01   |          |          | Ch?    |              |
```

---

<a id="summary"></a>

## 六、背诵

**PNP = 实战踩坑；UNP = 体系圣经；C++ 课 = 语言工程。三门各干各的，PNP 与 UNP 重叠最大，用本仓库当 UNP 索引。**
