# MikanOS · 学习计划

## 三阶段（约 8–12 周，业余节奏）

### 阶段 A · 现代启动链（Ch 1–2）🔴

**笔记：** [chapter-01-hello-world](./chapter-01-hello-world/) · [chapter-02-edk2-memmap](./chapter-02-edk2-memmap/)

**目标：** UEFI → EDK II → **内存 map** → 第一个 C++ 内核输出。

**前置：** [01 Day 1](../02-30days-os/day-01-boot-asm/) 完成（理解 boot sector / `hello, world` 体感）。

**产出：** 能解释 **UEFI 与 BIOS 软盘引导** 差异；能读 `EFI_MEMORY_DESCRIPTOR`。

### 阶段 B · 内核骨架（Ch 7–14）🔴

**笔记：** [chapter-07-interrupt-fifo](./chapter-07-interrupt-fifo/) · [chapter-08-memory](./chapter-08-memory/) · [chapter-11-timer-acpi](./chapter-11-timer-acpi/) · [chapter-13-multitask1](./chapter-13-multitask1/) · [chapter-14-multitask2](./chapter-14-multitask2/)

**目标：** 中断 · FIFO · **物理/线性内存** · 定时器 · **协作/抢占多任务**。

**对照 02-30days-os：** Day 5–16（IDT/PIC/多任务）— 同一概念，不同架构（APIC、64 bit）。

**产出：** 能画 MikanOS 任务切换与 01 `switch_task` 对照表。

### 阶段 C · 分页与 syscall（Ch 19–20）🔴

**笔记：** [chapter-19-paging](./chapter-19-paging/) · [chapter-20-syscall](./chapter-20-syscall/)

**目标：** **页表** · 用户/内核地址空间 · **系统调用门**。

**交叉：** [01-CSAPP](../01-CSAPP-3rd/) Ch 9 · [04-Linux-Kernel-Development](../04-Linux-Kernel-Development/) Ch 5/10 · [05-Understanding-Linux-Kernel](../../05-Understanding-Linux-Kernel/) Ch 2/9/10。

**产出：** 理解「TLPI 的 `mmap` 底下分页长什么样」的最小实现版。

---

## 与仓库其他模块

```
02 30天（BIOS/实模式启蒙）
    ↓
02 MikanOS（UEFI/64位/分页/syscall）
    ↓
08 TLPI（用户态 API）+ 05 LKD / 06 ULK（Linux 内核实现）
```

**不必：** 在 MikanOS 里复刻 Linux；**要：** 用最小 OS 验证 CSAPP/LKD 里的抽象。

---

## 避坑

| 坑 | 说明 |
|----|------|
| 跳过 02-30days-os 直接 MikanOS | 可以，但 **实模式→保护模式** 体感会缺一块 |
| WSL 开发环境 | 全书 **WSL2 + Ubuntu**；笔记以 **QEMU + OVMF** 为主（见 [SETUP](./SETUP.md)） |
| 整仓 copy os-from-zero | 用 **官方 tag + 本仓库 chapter/code 快照** |
| Ch 4–6 GUI | HFT 可 ⚪ 速读，保留 Ch 7+ |
