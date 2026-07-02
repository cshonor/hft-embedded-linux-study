# HFT / 嵌入式主线 · 学什么、先搁置什么

> **结论（定死）：** **C → [01 MikanOS](./01-mikan-os/) → Linux 内核 / DPDK → HFT**。[02 30 天 OS](./02-30days-os/) **彻底后置**，可当休闲拓展，**不必系统敲码**。  
> 与 [LEARNING-CHAIN](../../LEARNING-CHAIN.md) 对齐；09 子目录 **01 = MikanOS，02 = 30 天**。

---

## 〇、为什么前后说法会矛盾？

仓库里曾写 **「先 30 天启蒙 → 再 MikanOS」** — 那条路 **只适用于**：

| 适用人群 | 特征 |
|----------|------|
| **纯零基础** | 完全不懂计算机底层 |
| **目标** | 只想入门 OS 原理，建立笼统认知 |
| **职业** | **不做** 量化 / 服务器底层 / 嵌入式 Linux 开发 |

**缺点（对你无效）：** 16 位 BIOS / 软盘与现代 x86-64、UEFI、Linux **割裂**，对 HFT 主线 **复用极低**。

---

## 一、你的专属路线：跳过 30 天，直接 MikanOS

| 维度 | **01 MikanOS** | **02 30 天 haribote** |
|------|----------------|------------------------|
| 架构 | **UEFI + x86-64 长模式** | BIOS 软盘 + 16 位实模式 |
| 语言 | **C/C++ 为主**，少量 64 位 asm | 大量 16 位 asm |
| 核心知识 | **64 位分页、特权级、syscall** | GDT 分段、BIOS 中断 |
| HFT 路线 | **🔴 主攻** | **⚪ 后置 / 可不学** |

---

## 二、最终固定学习顺序（HFT 主线）

```text
1. 吃透 C 语言（+ CSAPP 机器码/缓存并行）

2. 直接学 01 MikanOS（07/01-mikan-os/）

3. Linux 内核 · 性能调优 · DPDK → 15 HFT
   ↔ 05-LKD · 08-TLPI · 14-DPDK

4. 02 30 天 OS：彻底后置，休闲拓展即可

5. 03 SysPerf · 04 BPF：同样后置 — 有内核/网络/交易系统可观测后再开
   （文件夹仍叫 03/04，执行顺序见 LEARNING-CHAIN）
```

**与通用零基础路线：**

```text
通用：02 30天启蒙 → 01 MikanOS → …
HFT： C → 01 MikanOS → 05/08/14 → 15  （02 可跳过；03/04 后置）
```

---

## 三、现阶段：优先 C，搁置 16 位引导

**暂时搁置：** 512 B 引导扇区、16 位实模式、`INT 0x13` 软盘读写。

**主投入：** [01-CSAPP-3rd](../../01-CSAPP-3rd/) · [06-The-Linux-Programming-Interface](../../06-The-Linux-Programming-Interface/) · [01-mikan-os](./01-mikan-os/)

---

## 四、汇编只学两类

| 类型 | 何时学 | 用途 |
|------|--------|------|
| **x86-64（CSAPP）** | 紧接 C | HFT 反汇编、缓存、延迟毛刺 |
| **ARM64** | 嵌入式飞控阶段 | 设备树、驱动 |

haribote 16 位：**不学 / 后置**。

---

## 五、若你已经在写 Day 3 haribote

| 情况 | 建议 |
|------|------|
| 已跑通黑屏 | **收工** — 转 [01-mikan-os/chapter-01-hello-world/](./01-mikan-os/chapter-01-hello-world/) |
| 纠结 GDT / INT 0x13 | **停** — MikanOS 走 UEFI |

---

## 六、一句话

**HFT 路线：C → 01 MikanOS → Linux/DPDK；02 30 天可不学；03/04 性能书等有系统可测再读。**

---

## 相关

| 文档 | 用途 |
|------|------|
| [01-mikan-os/README.md](./01-mikan-os/README.md) | **HFT OS 动手入口** |
| [02-30days-os/README.md](./02-30days-os/README.md) | 通用启蒙 / 休闲 |
| [LEARNING-CHAIN.md](../../LEARNING-CHAIN.md) | 全仓执行顺序（含 03/04 后置） |
