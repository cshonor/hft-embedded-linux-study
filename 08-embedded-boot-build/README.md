# 20 · U-Boot / Kernel / Build（嵌入式 Linux 构建）

> **读序：** *Embedded Linux Primer*（世界观）→ [*MELP* 3rd / Simmonds](https://www.packtpub.com/product/build-toolchain-yocto-third-edition/9781803234384)（实操）  
> **重合与跳章：** [FOUR-BOOKS-OVERLAP.md](./FOUR-BOOKS-OVERLAP.md)（含与 21 驱动书 C/D 的分工）

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
08-embedded-boot-build/
├── README.md                          ← 本文件
├── FOUR-BOOKS-OVERLAP.md              ← Primer / MELP / LDD3 / Madieu 分工
├── primer-system-overview/             ← Hallinan · 先读（世界观）
│   ├── README.md · OUTLINE.md         ← 19 章完整小节大纲
│   └── chapter-01-introduction/ … chapter-19-udev/
├── build-toolchain-yocto/
│   ├── OUTLINE.md                     ← MELP 21 章 · 后读（实操）
│   └── chapter-01-… / chapter-21-…
└── _scripts/
    └── scaffold-simmonds-melp.py
```

---

## Simmonds 第三版 · 快速入口

**全书大纲：** [build-toolchain-yocto/OUTLINE.md](./build-toolchain-yocto/OUTLINE.md)

| Section | 章 | 主题 |
|---------|-----|------|
| **S1** | 1–8 | 工具链、引导、内核、rootfs、Buildroot/Yocto |
| **S2** | 9–15 | 存储、OTA、驱动、原型板、init、电源 |
| **S3** | 16–18 | Python 打包、进程/线程、内存 |
| **S4** | 19–21 | GDB、perf/BPF、PREEMPT_RT |

**建议精读：** Ch 2–7、9、11、13、17–19、21（见 OUTLINE 标签表）。

---

## Embedded Linux Primer（Hallinan）· 先读

| | |
|--|--|
| 入口 | [primer-system-overview/README.md](./primer-system-overview/README.md) |
| 大纲 | [primer-system-overview/OUTLINE.md](./primer-system-overview/OUTLINE.md)（19 章 + 附录小节） |
| 定位 | **概念模型 / 启动与系统全貌**；驱动只入门 |

与 MELP 在 Bootloader / 内核编译 / rootfs 上有重叠 — Primer 懂了可跳过 MELP 对应重复章，直奔调试与构建系统。详见 [FOUR-BOOKS-OVERLAP](./FOUR-BOOKS-OVERLAP.md)。

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
| [19 ARM64](../07-arm-architecture/) | U-Boot/内核/设备树与 AArch64 汇编 |
| [21 Linux Device Drivers](../09-device-drivers-dt) | Ch 11 驱动交互 |
| [21 驱动+DT](../09-device-drivers-dt/) | Ch 3–4、11–09 驱动与设备树 |
| [04 LKD](../05-linux-kernel/) | 内核机制 |
| [07 TLPI](../03-linux-userspace-api/) | 进程/IPC/内存 |
| [08 MikanOS 等](../projects/P9-os-from-scratch/) | 自底向上对照 |

---

## 进度

- [x] Simmonds 3rd — 21 章脚手架 + OUTLINE
- [ ] 各章口述笔记（按 `{章号}` 或摘要驱动）
- [ ] Hallinan 目录
- [ ] WSL 实机构建记录（U-Boot / kernel / Buildroot）
