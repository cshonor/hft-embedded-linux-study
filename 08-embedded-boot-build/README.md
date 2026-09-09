# 08 · 嵌入式 Linux 构建：点亮一块板

> **本模块回答一个问题：** 一块裸板，怎么变成一台能进 shell 的 Linux 机器。
> **组织方式：** 目录是**任务节点**（要做出什么），不是书的章节。书降级为工具书，见 [`_refs/BOOK-MAP.md`](./_refs/BOOK-MAP.md)。
> **动手不在这里：** 实际操作步骤与踩坑记录放在 [`projects/P5-raspberry-pi-embedded/RASPBERRY-PI5-LABS.md`](../projects/P5-raspberry-pi-embedded/RASPBERRY-PI5-LABS.md)（Phase A→G）。本模块是**知识支撑**，那里是**执行器**。

---

## 任务节点

| # | 节点 | 交付物 | 对应动手 |
|---|------|--------|----------|
| 01 | [orientation](./01-orientation/) | 说清 SoC / RTOS / BSP / DT 是什么 | P5 · A |
| 02 | [toolchain](./02-toolchain/) | 交叉编出板子能跑的 aarch64 ELF | P5 · D1 |
| 03 | [u-boot](./03-u-boot/) | 画得出上电到 `start_kernel` 的链路 | P5 · B3 |
| 04 | [kernel-build](./04-kernel-build/) | 自编内核在板子上启动 | P5 · B1 |
| 05 | [rootfs](./05-rootfs/) | 出一份可启动的最小根文件系统 | P5 · B2 |
| 06 | [boot-to-shell](./06-boot-to-shell/) | 串口进 shell，说清 PID 1 怎么来的 | P5 · B2/B3 |
| 07 | [storage-ota](./07-storage-ota/) | 分区方案 + 可回退的升级流程 | — |

---

## 关于 Yocto（先说清楚，别走错路）

**主线走 Buildroot，不走 Yocto。**

| | Buildroot | Yocto |
|--|-----------|-------|
| 首个可启动镜像 | 几十分钟 | 数小时起 |
| 磁盘 | 几个 GB | 几十 GB |
| 概念栈 | Kconfig + make（**与内核同构**） | recipe / layer / bitbake |
| 适合 | 搞懂启动链、快速迭代 | 量产、多机型、长期维护 |

Yocto 是量产的**事实标准**，但**不是学习入口**。等真要做产品镜像再上，详见 [`05-rootfs`](./05-rootfs/)。

---

## 与 P5 的分工

```
08 / 09  ←  为什么这么做、怎么做对、查哪本书（本仓库的"知识层"）
   ↕ 互相指向
projects/P5  ←  具体命令、板上结果、踩坑记录（"执行层"）
```

每做一步 P5 的 Phase，回来对应的节点补「为什么」。两边都写才算吃透。

---

## 环境约定

| 项 | 约定 |
|----|------|
| 笔记 | Windows + 编辑器 |
| 构建/烧录 | **WSL**（Ubuntu），或直接在 Pi 上本地编 |
| 目标板 | Raspberry Pi 5（BCM2712 + RP1，aarch64） |
| 交叉前缀 | `aarch64-linux-gnu-` |
| 内核分支 | `rpi-6.12.y`（随官方调整） |

---

## 卡住时查哪本书

| 书 | 定位 |
|----|------|
| *Embedded Linux Primer*（Hallinan） | 概念模型 · 先搞清"为什么" |
| *Mastering Embedded Linux Programming* 3rd（Simmonds） | 全流程实操 · 查"怎么做" |

完整映射：[`_refs/BOOK-MAP.md`](./_refs/BOOK-MAP.md)
四本书分工：[`_refs/FOUR-BOOKS-OVERLAP.md`](./_refs/FOUR-BOOKS-OVERLAP.md)

---

## 模块交叉

| 模块 | 关系 |
|------|------|
| [09 驱动](../09-device-drivers-dt/) | 本模块让系统起来，09 让硬件能用 |
| [07 ARM 架构](../07-arm-architecture/) | 启动与 AArch64 汇编 |
| [05 内核](../05-linux-kernel/) / [05.5](../05.5-modern-kernel/) | 内核机制与 PREEMPT_RT |
| [03 TLPI](../03-linux-userspace-api/) | 用户态进程/IPC/内存 |
| [P5 树莓派实战](../projects/P5-raspberry-pi-embedded/) | 动手清单 |

---

## 进度

- [x] 去书本化重构：80 章书目录 → 7 个任务节点
- [x] 01-orientation（6 篇）
- [ ] 02-toolchain
- [ ] 03-u-boot（1 篇）
- [ ] 04-kernel-build
- [ ] 05-rootfs
- [ ] 06-boot-to-shell
- [ ] 07-storage-ota
