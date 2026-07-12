# 20 · U-Boot / Kernel / Build（嵌入式 Linux 构建）

> **主线：** [*Mastering Embedded Linux Programming*](https://www.packtpub.com/product/mastering-embedded-linux-programming-third-edition/9781803234384)（Chris Simmonds，**第 3 版**）  
> **补充：** *Embedded Linux Primer*（Hallinan）— 概念与历史视角，待建目录

---

## 本模块做什么

从 **交叉工具链 → Bootloader（U-Boot）→ 内核配置/编译 → 根文件系统 → Buildroot/Yocto** 走通「单板启动」全链路；并覆盖存储、init、驱动交互、调试与实时等架构决策。

| 能力 | 对应章节（Simmonds 3rd） |
|------|-------------------------|
| 工具链与交叉编译 | Ch 2 |
| U-Boot / 设备树 / 引导 | Ch 3–4 |
| rootfs / Buildroot / Yocto | Ch 5–8 |
| 存储与 OTA | Ch 9–10 |
| 用户空间 init / 服务 | Ch 13–14 |
| 进程/内存/调试/实时 | Ch 17–21 |

---

## 目录结构

```
20-UBoot-Kernel-Build/
├── README.md                          ← 本文件
├── mastering-embedded-linux-programming/
│   ├── OUTLINE.md                     ← 21 章 × 4 Section 大纲 + 精读标签
│   ├── chapter-01-getting-started/
│   ├── chapter-02-toolchain/
│   └── … chapter-21-real-time-programming/
├── _scripts/
│   └── scaffold-simmonds-melp.py      ← 重建章节目录
└── (embedded-linux-primer/)           ← Hallinan，待补充
```

---

## Simmonds 第三版 · 快速入口

**全书大纲：** [mastering-embedded-linux-programming/OUTLINE.md](./mastering-embedded-linux-programming/OUTLINE.md)

| Section | 章 | 主题 |
|---------|-----|------|
| **S1** | 1–8 | 工具链、引导、内核、rootfs、Buildroot/Yocto |
| **S2** | 9–15 | 存储、OTA、驱动、原型板、init、电源 |
| **S3** | 16–18 | Python 打包、进程/线程、内存 |
| **S4** | 19–21 | GDB、perf/BPF、PREEMPT_RT |

**建议精读：** Ch 2–7、9、11、13、17–19、21（见 OUTLINE 标签表）。

---

## Embedded Linux Primer（Hallinan）

第二本书，侧重嵌入式 Linux **概念模型**与经典流程；与 Simmonds 并行时：**Simmonds 动手、Hallinan 补概念**。目录待建。

---

## 环境约定

| 项 | 约定 |
|----|------|
| 笔记 | Windows + Cursor |
| 构建/烧录 | **WSL**（Ubuntu 等） |
| 脚本 | Windows 下 `py` |

---

## 模块交叉链接

| 模块 | 关系 |
|------|------|
| [19 ARM64](../19-ARM64-Architecture/) | U-Boot/内核/设备树与 AArch64 汇编 |
| [21 Linux Device Drivers](../21-Linux-Device-Drivers/) | Ch 11 驱动交互 |
| [22 Device Tree](../22-Device-Tree/) | Ch 3–4、12 设备树 |
| [04 LKD](../04-Linux-Kernel-Development/) | 内核机制 |
| [07 TLPI](../07-The-Linux-Programming-Interface/) | 进程/IPC/内存 |
| [08 MikanOS 等](../08-system-low-level-hands-on/) | 自底向上对照 |

---

## 进度

- [x] Simmonds 3rd — 21 章脚手架 + OUTLINE
- [ ] 各章口述笔记（按 `{章号}` 或摘要驱动）
- [ ] Hallinan 目录
- [ ] WSL 实机构建记录（U-Boot / kernel / Buildroot）
