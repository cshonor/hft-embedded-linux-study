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

> **当前：** Phase1 `00` digital-logic-cpu。下一站 Phase2：`01` C → `02` computer-systems。
