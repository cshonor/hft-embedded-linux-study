# hft-embedded-linux-study

> **GitHub：** [github.com/cshonor/hft-embedded-linux-study](https://github.com/cshonor/hft-embedded-linux-study)  
> **HFT 低延迟 Linux 底层** + **嵌入式 Linux 无人机飞控** 双线笔记与路线仓库。

**技术板块 `00`–`23`：** 顶层为**纯技术模块名**；**编号 = 学习顺序**。

- **定稿执行顺序：** [LEARNING-PATH-LOCKED.md](./LEARNING-PATH-LOCKED.md)  
- 链路摘要：[LEARNING-CHAIN.md](./LEARNING-CHAIN.md)  
- 板块对照：[CROSS-MODULE-GUIDE.md](./CROSS-MODULE-GUIDE.md)  
- 完整路线图：[HFT-READING-ROADMAP.md](./HFT-READING-ROADMAP.md)

---

## 相关仓库

| 仓库 | 用途 | 本仓对应 |
|------|------|----------|
| **[hft-embedded-linux-study](https://github.com/cshonor/hft-embedded-linux-study)** | 本仓：读序、OUTLINE、章节 scaffold | `00`–`23` |
| **[cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes)** | C / C++ 详细笔记与代码 | [01 C](./01-c-language/) · [06 C++](./06-cpp/) |
| [Computer-Networking](https://github.com/cshonor/Computer-Networking) | Socket 实战代码 | [15 network-sockets](./15-network-sockets/) |

```bash
git clone https://github.com/cshonor/hft-embedded-linux-study.git
```

---

## 执行顺序（锁定 · 编号=读序）

```
00 数字逻辑/CPU → 01 C → 02 计算机系统
 → 04 用户态 API（穿插 05 自制 OS / 06 C++）
 → 07 内核 + 09 MM
 → A 嵌入式 10–13  ‖  B HFT 15–21
 → 拓展 03 · 08 · 22 · 23 ·（兴趣）14
```

详情 → [LEARNING-PATH-LOCKED.md](./LEARNING-PATH-LOCKED.md)

| 文件夹 | 模块 |
|:------:|------|
| **00** | [digital-logic-cpu](./00-digital-logic-cpu/) — 硬件底层 |
| **01** | [c-language](./01-c-language/) — C |
| **02** | [computer-systems](./02-computer-systems/) — 计算机系统 |
| **03** | [computer-architecture](./03-computer-architecture/) — 体系结构（拓展） |
| **04** | [linux-userspace-api](./04-linux-userspace-api/) — 用户态 API |
| **05** | [os-from-scratch](./05-os-from-scratch/) — 自制 OS |
| **06** | [cpp](./06-cpp/) — C++ |
| **07** | [linux-kernel](./07-linux-kernel/) — 内核入门 |
| **08** | [linux-kernel-deep](./08-linux-kernel-deep/) — 内核深度（拓展） |
| **09** | [linux-mm](./09-linux-mm/) — 内核内存 |
| **10** | [arm-architecture](./10-arm-architecture/) — ARM / AArch64 |
| **11** | [embedded-boot-build](./11-embedded-boot-build/) — 启动与构建 |
| **12** | [device-drivers-dt](./12-device-drivers-dt/) — 驱动 + 设备树 |
| **13** | [embedded-projects](./13-embedded-projects/) — 嵌入式实战 |
| **14** | [motion-control](./14-motion-control/) — 运动控制 / 飞控 |
| **15** | [network-sockets](./15-network-sockets/) — Socket |
| **16** | [tcpip-protocols](./16-tcpip-protocols/) — TCP/IP |
| **17** | [kernel-networking](./17-kernel-networking/) — 内核网络 |
| **18** | [dpdk](./18-dpdk/) — DPDK |
| **19** | [systems-performance](./19-systems-performance/) — 系统性能 |
| **20** | [bpf-observability](./20-bpf-observability/) — BPF |
| **21** | [hft-engineering](./21-hft-engineering/) — HFT 工程 |
| **22** | [rust-quant](./22-rust-quant/) — Rust 量化 |
| **23** | [markets-microstructure](./23-markets-microstructure/) — 市场微观结构 |

---

## Project 驱动学习路线

> 不是"先读完书再做项目"，而是**项目本身就是学习路径**——卡住了翻书查对应模块，做完就自然学会了。

```
P1 CPU 模拟器 → P2 Shell+malloc → P3 并发 HTTP Server → P4 内核模块
 → P5 树莓派嵌入式（5 子项目）
 → P6 网络协议分析器 → P7 DPDK 转发+延迟剖析
 → P8 迷你撮合引擎（终极大作业）
```

| Project | 做什么 | 覆盖模块 | 前置 |
|:-------:|--------|:--------:|:----:|
| **P1** | Logisim/Verilog 搭 8-bit CPU（ALU+寄存器+FSM） | `00` | 无 |
| **P2** | C 写 mini shell（fork/exec/pipe）+ 自制 malloc/free | `01` `02` | P1 |
| **P3** | 并发 HTTP Server：C 版（epoll+线程池）→ C++ 重写版（RAII+模板） | `04` `05` `06` | P2 |
| **P4** | 可加载内核模块：字符设备 + kmalloc 追踪 + /proc 统计 | `07` `08.5` `09` | P3 |
| **P5** | 树莓派嵌入式 Linux 全链路（5 子项目见下） | `10`–`14` | P4 |
| **P6** | raw socket 抓包 + 逐层解析 + TCP 流重组 + eBPF 追踪 NAPI | `15` `16` `17` `17.5` | P3 |
| **P7** | DPDK packet forwarder + perf 火焰图 + bpftrace 延迟探针 | `18` `19` `20` | P6 |
| **P8** | 限价订单簿撮合引擎：无锁 ring buffer + 绑核/Hugepage + Rust 重写 | `21` `22` `23` | P4+P5+P7 |

### P5 子项目（树莓派嵌入式）

| 子项目 | 交付 | 模块 |
|:------:|------|:----:|
| P5a | QEMU 裸机 UART Hello World | `10` |
| P5b | U-Boot → kernel → rootfs 启动到 shell | `11` |
| P5c | I2C/SPI 传感器驱动 + 设备树 | `12` |
| P5d | 多线程传感器融合 + 延迟 p99 统计 | `13` |
| P5e | PID 姿态控制（可选） | `14` |

> **当前：** Phase1 `00` digital-logic-cpu。下一站 Phase2：`01` C → `02` computer-systems。
