# 《ARM64体系结构编程与实践》— 章节目录与阅读裁剪

> **作者：** 奔跑吧Linux社区 · **出版社：** 人民邮电出版社  
> **模块：** [arm64-programming-practice/](./README.md) · [19 总览](../README.md)  
> **代码：** [runninglinuxkernel/arm64_programming_practice](https://github.com/runninglinuxkernel/arm64_programming_practice)  
> **平台：** 树莓派 4B · **QEMU ARM64** — 类 **高端无人机 / 网关应用处理器** 实验环境

| 标签 | 含义 |
|------|------|
| **精读** | 嵌入式 Linux / 飞控支线必看 |
| **选读** | 与 [03 Hennessy](../../03-Computer-Architecture-6th/) Ch2 重叠，可后补 |
| **跳过** | 首遍可略（SVE 等） |

---

## 全书 23 章

| 章 | 标题 | 标签 | 文件夹 | 飞控/嵌入式关联 |
|----|------|------|--------|-----------------|
| **1** | ARM64体系结构基础知识 | **精读** | [ch01](./chapter-01-arm64-fundamentals/) | ARMv8/v9 · EL · A64 · 寄存器 |
| **2** | 搭建树莓派实验环境 | **精读** | [ch02](./chapter-02-raspberry-pi-lab/) | **QEMU + GDB** · BenOS 实验链 |
| **3** | A64指令集1——加载与存储 | **精读** | [ch03](./chapter-03-a64-load-store/) | Load/Store · MMIO 基础 |
| **4** | A64指令集2——算术与移位 | **精读** | [ch04](./chapter-04-a64-arithmetic-shift/) | 位运算 · 地址计算 |
| **5** | A64指令集3——比较与跳转 | **精读** | [ch05](./chapter-05-a64-compare-branch/) | 控制流 |
| **6** | A64指令集4——其他重要指令 | **精读** | [ch06](./chapter-06-a64-other-instructions/) | 补充指令 |
| **7** | A64指令集的陷阱 | **精读** | [ch07](./chapter-07-a64-traps/) | 一线踩坑总结 |
| **8** | GNU汇编器 | **精读** | [ch08](./chapter-08-gnu-assembler/) | 读 U-Boot/内核 `.S` |
| **9** | 链接器与链接脚本 | **精读** | [ch09](./chapter-09-linker-scripts/) | 接 [20 构建](../../20-UBoot-Kernel-Build/) |
| **10** | GCC内嵌汇编 | **精读** | [ch10](./chapter-10-gcc-inline-asm/) | 驱动/内核常见写法 |
| **11** | 异常处理 | **精读** | [ch11](./chapter-11-exception-handling/) | **飞控底层必备** |
| **12** | 中断处理 | **精读** | [ch12](./chapter-12-interrupt-handling/) | ISR · 顶半部/底半部概念 |
| **13** | GIC-V2 | **精读** | [ch13](./chapter-13-gic-v2/) | **中断控制器** — 传感器/电机 IRQ |
| **14** | 内存管理 | **精读** | [ch14](./chapter-14-memory-management/) | 页表 · 接 [06 Gorman](../../06-Linux-Virtual-Memory-Manager/) |
| **15** | 高速缓存基础知识 | 选读 | [ch15](./chapter-15-cache-basics/) | 对照 Hennessy |
| **16** | 缓存一致性 | 选读 | [ch16](./chapter-16-cache-coherency/) | 多核 SoC |
| **17** | TLB管理 | 选读 | [ch17](./chapter-17-tlb-management/) | 缺页/TLB miss |
| **18** | 内存屏障指令 | **精读** | [ch18](./chapter-18-memory-barriers/) | **HFT 无锁同源** |
| **19** | 合理使用内存屏障 | **精读** | [ch19](./chapter-19-barrier-usage/) | 驱动/飞控并发 |
| **20** | 原子操作 | **精读** | [ch20](./chapter-20-atomic-operations/) | `ldxr`/`stxr` |
| **21** | 操作系统相关话题 | **精读** | [ch21](./chapter-21-os-topics/) | 进 [20](../20-UBoot-Kernel-Build/) / [21 驱动](../../21-Linux-Device-Driver/) |
| **22** | 浮点与NEON | 选读 | [ch22](./chapter-22-fp-neon/) | 姿态解算可后接 [24](../../24-Motion-Control-Motor/) |
| **23** | 可伸缩矢量计算与优化 | 跳过 | [ch23](./chapter-23-sve-optimization/) | 首遍可略 |

---

## 推荐阅读顺序（无人机 / 嵌入式 Linux）

```
1–2   架构 + QEMU/树莓派环境
  ↓
3–7   A64 指令集（动手实验）
  ↓
8–10  GNU 汇编 / 链接 / 内联汇编
  ↓
11–14 异常 · 中断 · GIC · 内存管理   ← 飞控底层核心
  ↓
18–20 屏障 · 原子                    ← 与 HFT 无锁思维对齐
  ↓
21    OS 话题 → 开 20 U-Boot / 21 驱动
```

**与 Smith 汇编书并行建议：** Smith [Ch2–8、13、16、18](../chapter-02-programmers-model/) 可 **压缩** 或 **选读** — 汇编思维已有则 **直接从本书 Ch1 开 AArch64**。

---

← [本书导读](./README.md) · [19 模块](../README.md)
