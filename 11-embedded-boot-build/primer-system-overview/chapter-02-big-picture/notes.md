# Embedded Linux Primer 第 02 章 — The Big Picture

> 对应目录：`chapter-02-big-picture/`  
> 书：*Embedded Linux Primer*, 2nd ed — Christopher Hallinan  
> 大纲：[../OUTLINE.md](../OUTLINE.md) · **全章精读**（发行版节亦精读）

**优先级**：2.1–2.3 **精读**；2.4 精读（商业 vs DIY）  
**后置**：[Ch7 Bootloaders](../chapter-07-bootloaders/) · [Ch5 内核初始化](../chapter-05-kernel-initialization/) · [MELP](../../build-toolchain-yocto/) · [13 Pi Labs](../../../13-embedded-projects/RASPBERRY-PI5-LABS.md)

---

## 章节定位

全书**核心总览**：嵌入式定义、上电启动链、存储、内存模型、交叉开发、发行版。  
后续处理器 / 内核 / Bootloader / 驱动等章都挂在本章骨架上。先吃透再决定 MELP 哪些节可压缩。

---

## 2.1 Embedded or Not?（精读）

### 2.1.1 嵌入式常见特征

1. 专用硬件、固定业务，非通用 PC  
2. 人机交互极简（LED、串口；常无键鼠显示器）  
3. 资源受限：小 Flash/RAM，无机械硬盘  
4. 常电池供电，功耗紧  
5. 出厂预装软件，用户一般不能随意装包  
6. 无人值守；断电重启可自恢复  

（树莓派桌面套件偏「板级电脑」，仍用同一套 Boot→内核→rootfs 叙事。）

### 2.1.2 BIOS vs Bootloader

| 项目 | PC BIOS（及后继固件） | 嵌入式 Bootloader（如 U-Boot） |
|------|----------------------|--------------------------------|
| 平台 | x86 标准 PC | ARM / MIPS / Power 等自定义板 |
| 职责 | 自检、从盘引导 OS | 初始化 DDR / 串口 / 网口；从 Flash/TFTP 载内核与 DT；传 cmdline |
| 生命周期 | 固件常驻可再进 | **内核起来后资源释放**；要再进 Bootloader 通常只能复位 |
| 定制 | 相对标准 | **必须按板移植 / 裁剪** |

自定义板几乎都要移植 Bootloader；标准 ATCA/cPCI 等可能自带成熟方案。  

**今日对照（BIOS → UEFI）：** 功能类比成立，但不能把 U-Boot 当成 UEFI。完整对比 → [2.1-uboot-bios-uefi.md](./2.1-uboot-bios-uefi.md)。  
深挖 U-Boot → [Ch7](../chapter-07-bootloaders/notes.md)。

---

## 2.2 Anatomy of an Embedded System（精读）

### 2.2.1 硬件骨架（书中无线 AP 类示例）

- SoC：32 位 RISC，集成 UART / USB / 以太 MAC 等  
- 外设：NOR 或 NAND（固件）、SDRAM（运行）、RTC、无线模组、RS-232、网口  

对照今日：Pi5 = 应用核 + **RP1** 外设；存储常是 **microSD**（见 [Project #1 卡笔记](../../../13-embedded-projects/project-01-pi-driver-course/02-microsd-card-reader.md)），Flash 分区思想仍适用。

### 2.2.2 交叉开发环境

| 端 | 角色 |
|----|------|
| **主机** | 桌面 Linux：工具链、源码、镜像、TFTP / NFS 服务 |
| **目标板** | 跑裁剪后的 Linux |

典型连线：

- **串口**：控制台（`minicom` / `screen`，常 115200）— 看启动日志、敲命令  
- **网线**：同网段 — TFTP 下内核 / DTB，NFS 挂网络 rootfs  

### 2.2.3 上电四阶段（必须能默写）

```
上电 → Bootloader → 内核解压/初始化 → 挂载根文件系统 → init（用户态）
```

| 阶段 | 做什么 | 日志特征 |
|------|--------|----------|
| **1. Bootloader** | 初始化内存/串口/网；TFTP 或 Flash 取 `uImage`/`Image` + DTB；`bootm`/`booti` 交权 | U-Boot 提示符前的打印 |
| **2. 内核** | 解压进 RAM；硬件探测、`printk` | CPU / 内存 / MTD / 网卡 / 驱动刷屏 |
| **3. 根文件系统** | 开发期常 **NFS**；量产 Flash 上 JFFS2/UBIFS 等 | Linux **必须有 rootfs**；裸机 RTOS 无此硬需求 |
| **4. 用户态** | 首进程 `init`；脚本、getty/登录 | 进入 shell 即用户空间 |

板级验收对齐 Labs **Phase B**（编内核 / rootfs / 启动参数）。

### 2.2.4 两种执行上下文

| | **内核空间** | **用户空间** |
|--|--------------|--------------|
| 何时 | 启动至 `init` 前；之后含驱动、系统调用路径、**ISR** | `init` 及所有应用 |
| 权限 | 可直访硬件与物理内存（在内核模型内） | 经系统调用；独立虚拟地址空间 |
| 约束 | ISR **禁止睡眠** | 进程崩溃默认不毁内核/他进程 |

与 [Project #1 三层图](../../../13-embedded-projects/project-01-pi-driver-course/01-userspace-kernel-hardware.md)、[Linux vs RTOS · MMU](../chapter-01-introduction/1.1-linux-vs-rtos.md) 同一条线。

---

## 2.3 Storage Considerations（精读）

### 2.3.1 NOR Flash

- 可 **XIP** 片上执行；读快；擦除块大（如 64KB）；寿命约 10⁵ 量级（书中量级）  
- 用途：Bootloader、小内核  
- 注意：改写常需整块擦；不适合高频日志  

### 2.3.2 NAND Flash

- 容量大、成本低；**不能**直接执行，须载入 RAM；有坏块  
- 用途：根文件系统、大固件  

### 2.3.3 典型 Flash 分区（四层思想）

1. Bootloader + 环境/配置  
2. 内核镜像  
3. Ramdisk / 根文件系统  
4. 升级预留（或用户数据）  

深挖 MTD → [Ch10](../chapter-10-mtd-subsystem/notes.md)。

### 2.3.4 Flash 文件系统

普通 ext2/3 **不**直接适配 Flash 擦写特性。嵌入式常用 **JFFS2 / UBIFS** 等：磨损均衡、断电友好、常带压缩。  
（eMMC/SD 上量产也常见 ext4；介质不同，选型不同。）FS 章 → [Ch9](../chapter-09-file-systems/notes.md)。

### 2.3.5 平面内存 vs 虚拟内存

| 模型 | 行为 |
|------|------|
| 传统无 MMU RTOS | 平面物理地址，任务共享；野指针易整机挂 |
| Linux + MMU | 每进程独立虚拟地址；内核映射相对固定（书中 32 位例：用户/内核分界如 `0xC0000000` 叙事） |

打印栈 / `.data` / `.bss` 虚拟地址即可直观感到「不是物理平面」。深挖 → ULK / Gorman（书末推荐）。

### 2.3.6 交叉编译痛点

- 主机原生 gcc → x86（或宿主机）二进制，**不能**当 ARM 板程序用  
- **交叉编译器**：跑在主机，产出目标 ABI 的二进制  
- 头文件 / 库必须跟**目标**走；和主机库混链 → 跑不起来的「四不像」  

工具链实操 → [MELP Ch2](../../build-toolchain-yocto/chapter-02-toolchain/) · Labs **D1**。

---

## 2.4 Embedded Linux Distributions（精读）

| 路线 | 特点 |
|------|------|
| **商业发行版** | 工具链 + BSP + 支持；适合量产工业/通信；授权成本高 |
| **DIY**（Buildroot / OpenEmbedded·Yocto 等） | 免费、可裁到骨；门槛高，要自己拼工具链/内核/rootfs/包 |

**发行版本质（缺一不可）：** 交叉工具链 + 内核（及驱动）+ 系统库 + 用户态工具（常 BusyBox）+ rootfs 构建脚本 → 才能打出可运行固件。

本仓库：世界观用 Primer；落地构建用 [MELP / Yocto 目录](../../build-toolchain-yocto/)。

---

## 2.5 小结

1. 嵌入式受限、专用；**Bootloader 替代 BIOS**，且多数板要移植  
2. 启动链：**Bootloader → 内核 → rootfs → init**  
3. 存储：NOR/NAND 分工 + Flash 专用 FS（或 eMMC/SD 上的常规 FS）  
4. MMU 隔离内核/用户，稳定性远高于平面 RTOS  
5. 开发必须 **交叉环境**；固件来自商业或 DIY 发行体系  

### 书中拓展阅读

- *Linux Kernel Development*（内核基础）  
- *Understanding the Linux Virtual Memory Manager*（Gorman，虚拟内存）  

仓库对应：[07 内核](../../../07-linux-kernel/) · 内存管理书目链。

---

## 与本仓库的咬合

| 本章概念 | 落到哪里 |
|----------|----------|
| 串口 + SSH/网 | Pi 刷机后 Phase A；串口日志看 Boot/内核 |
| 四阶段启动 | Phase B；[Ch5](../chapter-05-kernel-initialization/) · [Ch6](../chapter-06-user-space-initialization/) · [Ch7](../chapter-07-bootloaders/) |
| 处理器 / SoC 选型背景 | [Ch3](../chapter-03-processor-basics/notes.md)（ARM 精读） |
| 交叉编译 | MELP · Labs D1 |
| 内核 vs 用户 | [01 三层图](../../../13-embedded-projects/project-01-pi-driver-course/01-userspace-kernel-hardware.md) · [04 TLPI](../../../04-linux-userspace-api/) |
| rootfs / BusyBox | [Ch11](../chapter-11-busybox/) · MELP rootfs 章 |

**下一章：** [Ch3 Processor Basics](../chapter-03-processor-basics/notes.md)（ARM/SoC 选读精读）。

---

## 参考

- Hallinan, *Embedded Linux Primer*, 2nd ed, Chapter 2  
- 大纲：[../OUTLINE.md](../OUTLINE.md) §第 2 章
