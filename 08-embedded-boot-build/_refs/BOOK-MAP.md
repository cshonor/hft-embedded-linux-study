# 书 → 任务节点 映射表

> **本模块已经去书本化。** 目录是「要做出什么」，不是「书的第几章」。
> 书降级为**工具书**：某一步卡住了，来这张表查该翻哪一章，而不是从头通读。
> 原书章节目录（80 章）已归档到 `_archive/`，只作溯源，不再作为学习主线。

---

## 四本书

| 代号 | 书 | 作者 | 内核基准 | 定位 |
|------|-----|------|----------|------|
| **B** | *Embedded Linux Primer*, 2nd | Hallinan | 2.6 | 概念模型 · 先查它搞清"为什么" |
| **A** | *Mastering Embedded Linux Programming*, 3rd | Simmonds | 4.x/5.x | 全流程实操 · 查"怎么做" |
| **D** | *Linux Device Drivers Development* | Madieu | 4.1–4.13 | 现代驱动写法（在 [09](../09-device-drivers-dt/)） |
| **C** | *Linux Device Drivers*, 3rd（LDD3） | Corbet / Rubini / Kroah-Hartman | **2.6.10** | 驱动原理补课（在 [09](../09-device-drivers-dt/)） |

四书分工与重合度：[FOUR-BOOKS-OVERLAP.md](./FOUR-BOOKS-OVERLAP.md)

---

## 按任务节点查书

| 任务节点 | 卡住时查 |
|----------|----------|
| [01-orientation](../01-orientation/) | **B** Ch1（Linux vs RTOS）· Ch2（启动全景）· Ch3（SoC/处理器）· Ch7（Bootloader） |
| [02-toolchain](../02-toolchain/) | **A** Ch2（工具链）· **B** Ch12–13（开发环境与工具） |
| [03-u-boot](../03-u-boot/) | **A** Ch3（Bootloader）· **B** Ch7（Bootloader）· **B** Ch2.1（U-Boot/BIOS/UEFI 对比，已收笔记） |
| [04-kernel-build](../04-kernel-build/) | **A** Ch4（配置与编译内核）· **B** Ch4（内核构建）· Ch5（内核初始化） |
| [05-rootfs](../05-rootfs/) | **A** Ch5（rootfs）· Ch6（选构建系统）· **B** Ch6（用户空间初始化）· Ch9（文件系统）· Ch11（BusyBox） |
| [06-boot-to-shell](../06-boot-to-shell/) | **A** Ch13（init）· Ch14（BusyBox/runit）· **B** Ch6（用户空间 init） |
| [07-storage-ota](../07-storage-ota/) | **A** Ch9（存储策略）· Ch10（OTA）· **B** Ch10（MTD 子系统） |

---

## 按需深入（不在主线上，遇到再说）

| 想搞清 | 查 |
|--------|-----|
| Yocto 到底怎么组织 | **A** Ch7（Yocto 开发）· Ch8（Yocto 内部机制） |
| 要不要上 Yocto / 与 Buildroot 怎么选 | **A** Ch6（选构建系统）· 见 [05-rootfs](../05-rootfs/) 里的取舍表 |
| 板子选型的工程判断 | **A** Ch12（原型开发板） |
| 内核/应用调试 | **A** Ch19（GDB）· Ch20（profiling/tracing）· **B** Ch14–15 |
| 电源管理 | **A** Ch15 |
| Python 打包进镜像 | **A** Ch16 |
| 进程/线程/内存（嵌入式视角） | **A** Ch17–18 |
| 实时性 / PREEMPT_RT | **A** Ch21 · **B** Ch17 |
| USB / udev（设备节点怎么冒出来） | **B** Ch18–19 |

---

## 关于 Yocto 的取舍（重要）

Yocto 是**量产**事实标准，但**不是学习入口**：

| | Buildroot | Yocto |
|--|-----------|-------|
| 出第一个可启动镜像 | 几十分钟 | 数小时起 |
| 磁盘占用 | ~几个 GB | 几十 GB |
| 概念栈 | Kconfig + make，与内核同构 | recipe / layer / bitbake，另学一套 |
| 适合 | **学明白启动链**、快速迭代 | 产品化、长期维护、多机型 |

**本模块主线走 Buildroot**（见 [05-rootfs](../05-rootfs/)）。Yocto 的 Ch6–8 等你真要做量产镜像再翻，不占主线时间。

---

## 归档

`_archive/` 下保留原书章节骨架（Primer 19 章 / MELP 21 章），仅供溯源：

- `_archive/primer-system-overview/` — Hallinan
- `_archive/build-toolchain-yocto/` — Simmonds
- 完整章节大纲：`OUTLINE-PRIMER.md` · `OUTLINE-MELP.md`
