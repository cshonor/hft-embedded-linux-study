## ② 内核源码树 · The Kernel Source Tree

按 **功能子系统** 划分的顶层目录 — 读 LKD / 对照源码时的 **根导航**。  
完整优先级 + **目录↔章节表** → [KERNEL-SOURCE-TREE-MAP.md](../../KERNEL-SOURCE-TREE-MAP.md)

#### 顶层速查

| 目录 | 内容 | 你常问的 |
|------|------|----------|
| **`arch/`** | 架构相关：x86、**arm64**、引导、平台初始化 | 树莓派 ARM64 相关 |
| **`drivers/`** | 设备驱动（体量最大）：网卡、显示、GPU、传感器、块设备 | 嵌入式驱动主战场 |
| **`fs/`** | **VFS** + ext4、tmpfs… | 文件系统 |
| **`include/`** | 头文件：声明、结构体、宏 | **先看头、再看 .c** |
| **`kernel/`** | 核心：调度、信号、锁、定时器… | **HFT 抖动地图** |
| **`mm/`** | 页、slab、VMA、缺页… | 双线重中之重 |
| **`net/`** | TCP/IP 协议栈 | **HFT 极高权重** |
| **`ipc/`** | 共享内存、消息队列、信号量 | IPC |
| **`lib/`** | 内核自用库函数 | 工具 |
| **`init/`** | 启动；**`start_kernel`** | 上电路线 |

#### 读代码规范

```
include/linux/     ← 结构体 / API 合同
       │
       ▼
kernel/  mm/  net/  fs/   ← 子系统实现
       │
       ▼
arch/<arch>/  ·  drivers/  ← 架构细节 / 某硬件
```

#### 双线优先级（摘要）

| 优先级 | 目录 |
|--------|------|
| **必精读（通用）** | `kernel/` · `mm/` · `net/` · `include/linux/` |
| **嵌入式加重** | `drivers/` · `arch/arm64/` |
| **按需** | `fs/` · `ipc/` · `lib/` · `init/`（知 `start_kernel` 即可） |

#### 与 LKD 章节（摘要）

| 目录 | LKD |
|------|-----|
| `kernel/` | Ch 3–5 · 7–11（调度/中断/同步/定时器） |
| `mm/` | Ch 12 · 15（+ 深读 [06 Gorman](../../../../06-Linux-Virtual-Memory-Manager/)） |
| `fs/` | Ch 13 |
| `drivers/` / 模块 | Ch 17 |
| `arch/` | Ch 2 概览 · Ch 7–8 路径 · Ch 19 |
| `net/` | 书内浅 → [13 Rosen](../../../../13-Linux-Kernel-Networking/) |

**Phase：** 本图主用于 **Phase4 读 LKD 时**；现在 Phase1–2 只需收藏，不必沉进 `drivers/`。见 [LEARNING-PATH-LOCKED](../../../../LEARNING-PATH-LOCKED.md)。

---
