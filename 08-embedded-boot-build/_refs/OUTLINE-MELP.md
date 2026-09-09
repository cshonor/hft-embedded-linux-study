# Mastering Embedded Linux Programming — 3rd ed · 全书大纲

> **Chris Simmonds** · Packt · 21 章 · 4 个 Section  
> 本目录为 **Simmonds / MELP 实操主线**；建议先读 [Embedded Linux Primer OUTLINE](./BOOK-MAP.md) 再建全局观。  
> 四书重合与跳章：[FOUR-BOOKS-OVERLAP.md](./FOUR-BOOKS-OVERLAP.md)

---

## 阅读标签说明

| 标签 | 含义 |
|------|------|
| **精读** | 嵌入式 Linux / 飞控支线核心；需写口述笔记 |
| **选读** | 有用但可压缩；抓架构与决策点即可 |
| **跳过** | 与当前主线弱相关或可由其他模块覆盖 |

---

## Section 1 · 嵌入式 Linux 的基础要素（Ch 1–8）

> 开发环境、工具链、引导、内核、根文件系统、Buildroot/Yocto 自动化构建。

| Ch | 目录 | 英文标题 | 核心内容 | 标签 |
|----|------|----------|----------|------|
| 1 | [chapter-01-getting-started](./BOOK-MAP.md) | Getting Started | 嵌入式 Linux 生态、项目初期选项 | 选读 |
| 2 | [chapter-02-toolchain](./BOOK-MAP.md) | Learning About Toolchains | 工具链组件、crosstool-NG、交叉编译、静态/共享库 | 精读 |
| 3 | [chapter-03-bootloader](./BOOK-MAP.md) | All About Bootloaders | Bootloader 作用、引导顺序、设备树、U-Boot 构建/移植/Falcon | 精读 |
| 4 | [chapter-04-configuring-building-kernel](./BOOK-MAP.md) | Configuring and Building the Kernel | Kconfig/Kbuild、设备树与模块编译 | 精读 |
| 5 | [chapter-05-building-root-filesystem](./BOOK-MAP.md) | Building a Root Filesystem | 根文件系统组件、目录布局、设备节点、NFS | 精读 |
| 6 | [chapter-06-choosing-build-system](./BOOK-MAP.md) | Choosing a Build System | Buildroot vs Yocto、二进制分发 | 精读 |
| 7 | [chapter-07-developing-with-yocto](./BOOK-MAP.md) | Developing with Yocto | devtool、Recipe、可扩展 SDK、定制发行版 | 精读 |
| 8 | [chapter-08-yocto-under-the-hood](./BOOK-MAP.md) | Yocto Under the Hood | 元数据分层、BitBake 语法、排错 | 选读 |

**建议顺序：** Ch 2 → 3 → 4 → 5 → 6 → 7；Ch 1 可速览；Ch 8 在用过 Yocto 后再读。

---

## Section 2 · 系统架构和设计决策（Ch 9–15）

> 存储、OTA、驱动交互、原型板、init/systemd、服务监督、电源。

| Ch | 目录 | 英文标题 | 核心内容 | 标签 |
|----|------|----------|----------|------|
| 9 | [chapter-09-storage-strategy](./BOOK-MAP.md) | Creating a Storage Strategy | NOR/NAND、JFFS2/UBIFS、ext4/F2FS、只读/临时 FS | 精读 |
| 10 | [chapter-10-field-software-updates](./BOOK-MAP.md) | Field Software Updates | OTA 健壮性、Mender/balena、原子更新 | 选读 |
| 11 | [chapter-11-device-drivers-interaction](./BOOK-MAP.md) | Interacting with Device Drivers | 内核驱动与用户空间访问方式 | 精读 |
| 12 | [chapter-12-prototyping-dev-boards](./BOOK-MAP.md) | Prototyping with Breakout Boards | BeagleBone、设备树定制、SPI/NMEA 探测 | 选读 |
| 13 | [chapter-13-booting-init](./BOOK-MAP.md) | Booting Up — init | BusyBox init、SysV init、systemd | 精读 |
| 14 | [chapter-14-busybox-runit](./BOOK-MAP.md) | Starting with BusyBox runit | 服务划分、进程监督、日志 | 选读 |
| 15 | [chapter-15-power-management](./BOOK-MAP.md) | Power Management | DVFS、空闲状态、挂起 | 选读 |

**交叉链接：** Ch 3/4 ↔ [19 ARM64](../../07-arm-architecture) · Ch 11–12 ↔ [21 驱动+DT](../../09-device-drivers-dt)

---

## Section 3 · 编写嵌入式应用程序（Ch 16–18）

| Ch | 目录 | 英文标题 | 核心内容 | 标签 |
|----|------|----------|----------|------|
| 16 | [chapter-16-packaging-python](./BOOK-MAP.md) | Packaging Python | distutils/pip/venv/conda/Docker 部署 | 跳过 |
| 17 | [chapter-17-processes-threads](./BOOK-MAP.md) | Processes and Threads | IPC、多线程、ZeroMQ、调度策略 | 精读 |
| 18 | [chapter-18-managing-memory](./BOOK-MAP.md) | Managing Memory | 虚拟内存、地址空间、泄漏检测 | 精读 |

**交叉链接：** Ch 17–18 ↔ [07 TLPI](../../03-linux-userspace-api) · [04 LKD](../../05-linux-kernel)

---

## Section 4 · 调试和优化性能（Ch 19–21）

| Ch | 目录 | 英文标题 | 核心内容 | 标签 |
|----|------|----------|----------|------|
| 19 | [chapter-19-gdb-debugging](./BOOK-MAP.md) | Debugging with GDB | gdbserver、core dump、内核调试 | 精读 |
| 20 | [chapter-20-profiling-tracing](./BOOK-MAP.md) | Profiling and Tracing | perf、ftrace、LTTng、BPF、Valgrind | 选读 |
| 21 | [chapter-21-real-time-programming](./BOOK-MAP.md) | Real-time Programming | PREEMPT_RT、cyclictest、调度延迟 | 精读 |

**交叉链接：** Ch 21 ↔ 24 飞控实时（若存在）

---

## 精读章节速查（13 章）

`2, 3, 4, 5, 6, 7, 9, 11, 13, 17, 18, 19, 21`

---

## 口述笔记约定

与模块 19 相同：每章 `notes/section-0-本章完整概述.md` + 按小节拆分；用户提供章/节摘要后由 Agent 写入。
