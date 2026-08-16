# Project #1 · 用户态 / 内核 / 硬件 三层图

> 视频课开篇架构图笔记 · 对齐 [RASPBERRY-PI5-LABS §1.5](../RASPBERRY-PI5-LABS.md)

```
 Unprivileged          Privileged              Hardware
┌─────────────┐  mem  ┌─────────────┐  HAL  ┌─────────────┐
│ User Space  │·······│   Kernel    │·······│  Hardware   │
│ 桌面 / C 程序 │       │ 进程·内存·驱动 │       │ CPU RAM GPIO│
│ Shell …     │       │ FS · 网络栈  │       │ I2C SPI 盘  │
└─────────────┘       └─────────────┘       └─────────────┘
```

---

## 三层各干什么

| 层 | 图中例子 | 你现在学什么 |
|----|----------|--------------|
| **User Space** | 桌面、C/Python、Shell | [01 C](../../../01-c-language/) · [03 TLPI](../../../03-linux-userspace-api/) · Labs **Phase A** |
| **Kernel** | 进程/内存管理、**设备驱动**、FS、网络栈；中断处理也在这里 | [05 内核](../../../05-linux-kernel/) · [09 驱动](../../../09-device-drivers-dt/) · Labs **Phase C** |
| **Hardware** | CPU、RAM、GPIO、I2C、SPI、存储 | [00 硬件词汇](../../../17-computer-architecture/) · Pi5 上还有 **RP1** 管外设 |

普通应用**不能**直接摸寄存器；要碰硬件，代码通常落在 **Kernel**（驱动 / 中断），用户态只通过系统调用或 `/dev` 间接访问。

---

## 两条虚线（比三块方框更重要）

### 1. Memory Boundary（用户态 ↔ 内核）

- CPU 有 **特权级**：用户态 unprivileged，内核 privileged。
- 用户进程**看不到**内核地址空间；非法访问 → 缺页 / 被杀。
- 合法过界方式：`read` / `write` / `ioctl` / `mmap` 等系统调用 → 陷入内核 → 再回用户态。

**驱动课要建立的直觉：** 你的 `.ko` 跑在虚线**右边（特权侧）**；用户态程序只和 `/dev/xxx` 说话。

### 2. HAL（内核 ↔ 硬件）

- 图中 **HAL** = 内核侧对硬件的抽象（驱动 + 子系统），不是 MCU 裸机里的那个「厂商 HAL 库」。
- 内核通过驱动读写 MMIO / 处理 IRQ；用户态不直接碰 GPIO 寄存器（除非刻意用 `/dev/mem` 等旁路，生产上少用）。

**Pi5：** GPIO 等外设常经 **RP1**；视频若按 Pi4 讲寄存器，以[官方文档](../RASPBERRY-PI5-LABS.md)为准。

---

## 和本仓库的对应（一句话）

| 仓库模块 | 落在图的哪一层 |
|----------|----------------|
| 03 用户态 API、Phase A | 左：User Space |
| 05 内核地图、12 字符/平台驱动、`.ko` | 中：Kernel（本课主战场） |
| 00 架构、板级 GPIO/总线 | 右：Hardware |
| 13 DPDK / UIO·VFIO | **故意绕开**中间标准驱动路径（旁路），先把标准三层搞清再谈 |

---

**自检**

- [ ] 能口述：为何用户态不能直接写 GPIO 寄存器  
- [ ] 能指出：字符驱动的 `open/read` 回调跑在哪一层  
- [ ] 能说出两条虚线各防止什么（乱碰内核内存 / 乱碰硬件细节）  
- [ ] 能区分：DT 不是驱动；`.ko` / built-in 都是内核代码 — [Primer 8.2](../../../08-embedded-boot-build/primer-system-overview/chapter-08-device-driver-basics/8.2-why-drivers-in-kernel.md)  
- [ ] 能复述：驱动管初始化 + 读写 + **中断** + 释放 — [Primer 8.3](../../../08-embedded-boot-build/primer-system-overview/chapter-08-device-driver-basics/8.3-driver-lifecycle-and-irq.md)
