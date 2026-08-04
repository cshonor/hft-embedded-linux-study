# Embedded Linux Primer, 2nd ed · 全书大纲

> **Christopher Hallinan** · Prentice Hall · 19 章 + 附录  
> 中译：《嵌入式 Linux 基础教程（第 2 版）》  
> 读序：本书 → [MELP](../build-toolchain-yocto/) · 重合说明：[FOUR-BOOKS-OVERLAP](../FOUR-BOOKS-OVERLAP.md)

## 阅读标签

| 标签 | 含义 |
|------|------|
| **精读** | 嵌入式主线必懂（启动链、存储、交叉环境、实时） |
| **选读** | 有用可压缩；或处理器/USB 等按需 |
| **跳过/速览** | 过时标准（LSB）或交由 MELP/21 深挖 |

---

## 第 1 章 Introduction — [chapter-01-introduction](./chapter-01-introduction/)

| 节 | 重点 | 标签 |
|----|------|------|
| 1.1 Why Linux? | 硬件兼容、无版权费、协议多、社区；扩展 [Linux vs RTOS](./chapter-01-introduction/1.1-linux-vs-rtos.md) | 选读 |
| 1.2 Embedded Linux Today | 市场与消费/通信落地 | 速览 |
| 1.3 Open Source and the GPL · 1.3.1 Free vs Freedom | 「免费啤酒」vs「自由」；GPL 传染 | 精读 |
| 1.4 Standards…（LSB / LF / CGL / Moblin / SA Forum） | LSB **工程可忘掉**；FAQ [为何书讲 LSB / vs POSIX](./chapter-01-introduction/1.4-lsb-vs-posix.md)；对照 [TLPI Ch1](../../04-linux-userspace-api/chapter-01-introduction/) | 速览 |

| 1.5 Summary | — | — |

---

## 第 2 章 The Big Picture — [chapter-02-big-picture](./chapter-02-big-picture/)

| 节 | 重点 | 标签 |
|----|------|------|
| 2.1 Embedded or Not? · 2.1.1 BIOS vs Bootloader | PC 固件 vs 嵌入式引导；今日读 [U-Boot vs BIOS vs UEFI](./chapter-02-big-picture/2.1-uboot-bios-uefi.md) | **精读** |
| 2.2 Anatomy… · 2.2.1–2.2.5 | 硬件框图；上电→U-Boot→内核→`init` | **精读** |
| 2.3 Storage… · 2.3.1–2.3.8 | Flash/NAND/NOR、分区、FS、地址空间、内核/用户态、交叉开发架构 | **精读** |
| 2.4 Embedded Linux Distributions | 商业发行版 vs 自制（LFS/Buildroot 思路） | 精读 |

→ 与 MELP 启动/rootfs 重叠章：先吃透本章再决定是否跳过 MELP 对应节。

---

## 第 3 章 Processor Basics — [chapter-03-processor-basics](./chapter-03-processor-basics/)

| 节 | 重点 | 标签 |
|----|------|------|
| 3.1 Stand-Alone（Power 970 / Pentium M / Atom / MPC7448…） | 独立 CPU + 芯片组时代 | 速览 |
| 3.2 Integrated SoC（PowerQUICC/QorIQ、MIPS、**ARM/i.MX**…） | SoC 主流；树莓 BCM 归类见 [Pi = SoC](./chapter-03-processor-basics/3.2-raspberry-pi-is-soc.md) | 选读（ARM 精读） |
| 3.3 小众架构 · 3.4 CompactPCI / ATCA | 工业机箱 | 按需 |

---

## 第 4 章 The Linux Kernel: A Different Perspective — [chapter-04-kernel-construction](./chapter-04-kernel-construction/)

> 工程视角：编译与目录结构，**不是** LKD 级原理。

| 节 | 重点 | 标签 |
|----|------|------|
| 4.1 Background | 版本号、仓库、`git` 取源码 | 精读 |
| 4.2 Kernel Construction | 顶级目录；编译流；`vmlinux` / zImage / uImage | **精读** |
| 4.3 Kernel Build 系统 | `.config`、menuconfig、`make` 目标 | **精读** |
| 4.4–4.6 | 配置选项、官方文档、定制清单 | 选读 |

---

## 第 5 章 Kernel Initialization — [chapter-05-kernel-initialization](./chapter-05-kernel-initialization/)

| 节 | 重点 | 标签 |
|----|------|------|
| 5.1 复合镜像 / piggy | Image、`head.o`、二级引导 | **精读** |
| 5.2 执行流 | `head.S` → `start_kernel()` / `main.c` | **精读** |
| 5.3 命令行 · `__setup` | 启动参数解析 | 精读 |
| 5.4 `__initcall` 分级 | 子系统初始化顺序 | 精读 |
| 5.5 init 内核线程 | 内核态收尾 | 精读 |

---

## 第 6 章 User Space Initialization — [chapter-06-user-space-initialization](./chapter-06-user-space-initialization/)

| 节 | 重点 | 标签 |
|----|------|------|
| 6.1 RootFS · FHS · 最小 rootfs | BusyBox/LFS 核心 | **精读** |
| 6.2 内核挂载 root | 挂载步骤 | **精读** |
| 6.3 `init` / inittab | 首个用户态进程 | **精读** |
| 6.4 initrd · 6.5 initramfs | 临时根 / 现代方案 | 精读 |

---

## 第 7 章 Bootloaders — [chapter-07-bootloaders](./chapter-07-bootloaders/)

| 节 | 重点 | 标签 |
|----|------|------|
| 7.1–7.2 | Bootloader 职责与硬件难点 | **精读** |
| 7.3 U-Boot 使用 | 获取/编译/配置/串口网络命令 | **精读** |
| 7.4 移植实战 | 板级移植要点 | 精读（落地跟 MELP） |
| 7.5 DTB | DTS→dtb | **精读**（接 [21 驱动/DT](../../12-device-drivers-dt/)） |
| 7.6 LILO/GRUB | PC 引导对比 | 速览 |

---

## 第 8 章 Device Driver Basics — [chapter-08-device-driver-basics](./chapter-08-device-driver-basics/)

| 节 | 重点 | 标签 |
|----|------|------|
| 8.1 模块 · 最简字符驱动 | 概念预告 | 选读 |
| 8.2 insmod/rmmod/lsmod/depmod | 模块工具 | 选读 |
| 8.3 设备号 · mknod · fops | 节点与接口 | 选读 |

→ **深入交给 [21 LDD3 + Madieu](../../12-device-drivers-dt/)**，此处勿当驱动主教材。

---

## 第 9 章 File Systems — [chapter-09-file-systems](./chapter-09-file-systems/)

| 节 | 重点 | 标签 |
|----|------|------|
| 9.1 VFS · ext2/3/4 | 抽象层与磁盘 FS | 精读 |
| 9.6 JFFS2 | Flash 日志 FS | 精读 |
| 9.9 proc / sysfs | 伪文件系统 | 精读 |

---

## 第 10 章 MTD Subsystem — [chapter-10-mtd-subsystem](./chapter-10-mtd-subsystem/)

| 节 | 重点 | 标签 |
|----|------|------|
| 10.1 MTD 架构 | 闪存抽象 | **精读** |
| 10.2 Flash 分区 | 两种定义方式 | 精读 |
| 10.4 UBIFS | 大容量闪存 FS | 精读 |

---

## 第 11 章 BusyBox — [chapter-11-busybox](./chapter-11-busybox/)

| 节 | 重点 | 标签 |
|----|------|------|
| 11.1 作用 | 精简命令集 | **精读** |
| 11.2 交叉编译 | 构建 BusyBox | 精读 |
| 11.3 busybox init | 初始化脚本 | 精读 |

---

## 第 12 章 Embedded Development Environment — [chapter-12-development-environment](./chapter-12-development-environment/)

| 节 | 重点 | 标签 |
|----|------|------|
| 12.1 交叉编译流程 | 主机↔目标板 | **精读** |
| 12.3 TFTP / NFS / 串口 | 三大开发服务；**NFS root 开发神器** | **精读** |

---

## 第 13 章 Development Tools — [chapter-13-development-tools](./chapter-13-development-tools/)

| 节 | 重点 | 标签 |
|----|------|------|
| 13.1 GDB 远程 | 用户态远程调试基础 | 精读 |
| 13.4 strace/ltrace/内存检测 | 运行时诊断 | 选读 |
| 13.5 readelf/objdump/addr2line | 二进制分析 | 选读 |

---

## 第 14 章 Kernel Debugging Techniques — [chapter-14-kernel-debugging](./chapter-14-kernel-debugging/)

| 节 | 重点 | 标签 |
|----|------|------|
| 14.2 KGDB | 远程内核调试 | 选读 |
| 14.4 JTAG | 硬件调试 | 选读 |
| 14.6 无法开机排错 | 启动失败路径 | 精读 |

---

## 第 15 章 Debugging Embedded Applications — [chapter-15-debugging-applications](./chapter-15-debugging-applications/)

| 节 | 重点 | 标签 |
|----|------|------|
| 15.2 gdbserver | 跨板远程；多进程/线程技巧 | 精读 |

---

## 第 16 章 Open Source Build Systems — [chapter-16-open-source-build-systems](./chapter-16-open-source-build-systems/)

| 节 | 重点 | 标签 |
|----|------|------|
| 16.2 Scratchbox | 历史容器方案 | 速览 |
| 16.3 Buildroot | 极简构建 | **精读**（落地跟 MELP） |
| 16.4 OpenEmbedded / Yocto | BitBake、Recipe | **精读**（落地跟 MELP） |

---

## 第 17 章 Linux and Real Time — [chapter-17-linux-and-real-time](./chapter-17-linux-and-real-time/)

| 节 | 重点 | 标签 |
|----|------|------|
| 17.1 软/硬实时 · 调度与延迟 | 概念 | **精读**（HFT / 飞控） |
| 17.2 抢占模型 · PREEMPT_RT | 实时补丁路径 | **精读** |
| 17.4 ftrace · 中断屏蔽延迟 | 测量 | 精读 |

---

## 第 18 章 Universal Serial Bus — [chapter-18-usb](./chapter-18-usb/)

| 节 | 重点 | 标签 |
|----|------|------|
| USB 拓扑 · 存储/HID/网卡 · usbmon | 子系统与抓包 | 选读 |

---

## 第 19 章 udev — [chapter-19-udev](./chapter-19-udev/)

| 节 | 重点 | 标签 |
|----|------|------|
| 19.1 热插拔 · 19.4 规则 | 设备节点管理 | 选读 |
| BusyBox mdev | 轻量替代 | 选读 |

---

## 附录（无独立目录 · 查表即可）

| 附录 | 内容 |
|------|------|
| A | GPL 全文 |
| B | U-Boot 常用命令 |
| C | BusyBox 内置命令 |
| D | SDRAM 硬件设计 |
| E | 开源社区资源 |
| F | BDI JTAG 配置示例 |

---

## 最短精读路径（嵌入式 + 低延迟）

```
Ch2 全景 → Ch5–7 启动链（内核/用户态/U-Boot）
  → Ch4 编译结构 → Ch6/9–11 rootfs·FS·MTD·BusyBox
  → Ch12 NFS 开发环境 → Ch16 Buildroot/Yocto（概念）
  → Ch17 实时
  → Ch8 仅速览 → 转 21 驱动
```

然后进 [MELP OUTLINE](../build-toolchain-yocto/OUTLINE.md) 动手；启动/rootfs 已懂可跳 MELP 重复章。
