# Bootlin 训练材料要点 — 内核子系统

> Bootlin 公开培训讲义摘要 + 实验操作清单。
> 来源: https://bootlin.com/docs/
> 每节课整理：讲义要点 + 动手实验步骤 + 与旧书差异 + 自测题

## 主要课程

### Linux 内核
- [x] [内核架构概述](01-kernel-architecture.md) — 内核空间/用户空间、syscall 路径、与 LKD3 差异
- [x] [进程管理与调度](02-process-scheduling.md) — task_struct、CFS/EEVDF、cgroup v2
- [x] [中断与并发同步](03-interrupts-synchronization.md) — IRQ stack、threaded IRQ、per-CPU
- [x] [设备驱动模型](04-device-driver-model.md) — bus/device/driver、probe 链、sysfs

### 嵌入式 Linux
- [x] [Bootloader (U-Boot)](05-u-boot.md) — SPL/U-Boot 二阶段、bootcmd、bootz/booti
- [x] [设备树](06-device-tree.md) — DTS/DTB、compatible、phandle、overlay
- [x] [Buildroot / Yocto](07-buildroot-yocto.md) — rootfs 构建、包管理、SDK 生成
- [x] [实时性 (PREEMPT_RT)](08-preempt-rt.md) — threaded IRQ、priority inheritance、raw spinlock

### ARM64
- [x] [ARM64 架构基础](09-arm64-architecture.md) — EL0-EL3、异常级别、TLB、cache 维护
- [x] [ARM64 内核启动流程](10-arm64-boot-flow.md) — head.S→start_kernel→rest_init、页表建立

> 每节课整理：讲义要点 + 动手实验步骤 + 与旧书差异 + 自测题
