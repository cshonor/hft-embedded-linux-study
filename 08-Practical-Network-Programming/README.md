# Practical Network Programming — 陈硕 · muduo 网络实战（外部仓库）

**定位：** **C++ 网络服务骨架** — 粘包、自连接、Netcat、TTCP、epoll 等 **实验 + 坑点**；先动手写出能跑的服务，再回头啃 UNP 的 API 与协议细节。

**文件夹 `10`** · 外部书目 **PNP** · [返回总清单](../READING-LIST.md#补充陈硕-pnp--muduo-实战10-文件夹)

> **与 UNP 分工：** [PNP](https://github.com/cshonor/Computer-Networking/tree/main/PNP) = 工程向实验；[UNP Vol.1](../09-UNP-Vol1/) = Stevens 系统化 API。  
> **推荐顺序：** `08` TLPI → `07` 自制系统 → **`17` C++（至少 M1）** → **本模块 `10`** → `11` UNP → `12`–`14` 网络

## 笔记仓库（外部）

**仓库：** [cshonor/Computer-Networking](https://github.com/cshonor/Computer-Networking)

| 入口 | 链接 |
|------|------|
| PNP 根目录 | [PNP/](https://github.com/cshonor/Computer-Networking/tree/main/PNP) |
| 实验大纲 | [OUTLINE.md](https://github.com/cshonor/Computer-Networking/blob/main/PNP/OUTLINE.md) |
| 学习进度 | [study.md](https://github.com/cshonor/Computer-Networking/blob/main/PNP/study.md) |
| 源码约定 | [code/README.md](https://github.com/cshonor/Computer-Networking/blob/main/PNP/code/README.md) |

## 目录结构（外部仓库）

```
PNP/
├─ README.md · OUTLINE.md · study.md
└─ code/
   └─ {实验名}/
      ├─ notes.md          # 坑点 + UNP 互链
      ├─ original_cpp/     # 课程 C++ 版
      ├─ original_c/       # POSIX C（可选）
      └─ rewrite_rust/     # Rust 对照（可选）
```

## HFT 关联

| 主题 | 标签 | 为什么先读 |
|------|------|------------|
| epoll / 多路复用实战 | 🔴 必读 | 收多路行情前的 **骨架代码** |
| 粘包 / 半包 / 缓冲区 | 🔴 必读 | 二进制协议解析前置 |
| TTCP / 延迟粗测 | 🟡 选读 | 建立 RTT 直觉，后接 SysPerf Ch10 |
| muduo Reactor 模型 | 🟡 选读 | 理解线程 + event loop，对接 `13` HFT 架构 |
| Netcat / 自连接等工具实验 | 🟡 选读 | 排查网络问题的小工具 |

**读完本模块再开 UNP：** API 名称不再抽象 — 你知道 `epoll_wait` 在服务里长什么样。

## 为何不在本仓库展开

实验笔记与 `code/` 已在 [Computer-Networking/PNP](https://github.com/cshonor/Computer-Networking/tree/main/PNP) 维护；本仓库只做 **HFT 学习链索引** 与阅读顺序编排。

## 交叉阅读

- **下一步 · API 系统化：** [09-UNP-Vol1](../09-UNP-Vol1/)
- 协议语义 → [10-TCP-IP-Illustrated-Vol1](../10-TCP-IP-Illustrated-Vol1/)
- 内核实现 → [11-Linux-Kernel-Networking](../11-Linux-Kernel-Networking/)
- 程序员视角 → [01-CSAPP Ch11](../01-CSAPP-3rd/chapter-11-network-programming/)
- 系统底层 → [07-system-low-level-hands-on](../07-system-low-level-hands-on/)
- 工程落地 → [15-HFT-Low-Latency-Practice](../15-HFT-Low-Latency-Practice/)
