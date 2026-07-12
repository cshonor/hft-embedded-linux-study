# 《ARM64体系结构编程与实践》

> **奔跑吧Linux社区** · 人民邮电出版社 · **模块 19 · AArch64 实战主书**  
> **实验代码：** [github.com/runninglinuxkernel/arm64_programming_practice](https://github.com/runninglinuxkernel/arm64_programming_practice)  
> **平台：** 树莓派 4B · **QEMU ARM64**（类无人机应用处理器环境）

---

## 定位

| | |
|---|---|
| **补什么** | **ARMv8/v9 · A64 64 位指令** · 异常/中断 · **GIC** · **内存管理** |
| **与 Smith 关系** | [Smith v4T/v7-M 汇编](../chapter-02-programmers-model/) = 汇编思维入门；**本书 = AArch64 主战场** |
| **飞控/无人机** | 异常 · GIC · MM · 屏障/原子 — 对接 [21 驱动](../../21-Linux-Device-Driver/) · [24 飞控](../../24-Motion-Control-Motor/) |
| **下一步** | [20 U-Boot/构建](../../20-UBoot-Kernel-Build/) |

📋 **章节目录与裁剪** → [OUTLINE.md](./OUTLINE.md)

---

## 章节目录（Ch 1–23）

| 章 | 文件夹 | 标签 |
|----|--------|------|
| 1 | [chapter-01-arm64-fundamentals](./chapter-01-arm64-fundamentals/) | 精读 |
| 2 | [chapter-02-raspberry-pi-lab](./chapter-02-raspberry-pi-lab/) | 精读 |
| 3–7 | [ch03](./chapter-03-a64-load-store/) … [ch07](./chapter-07-a64-traps/) | **A64 指令集** · 精读 |
| 8–10 | [ch08](./chapter-08-gnu-assembler/) … [ch10](./chapter-10-gcc-inline-asm/) | 工具链 · 精读 |
| 11–14 | [ch11](./chapter-11-exception-handling/) … [ch14](./chapter-14-memory-management/) | **异常/GIC/MM** · 精读 |
| 15–17 | [ch15](./chapter-15-cache-basics/) … [ch17](./chapter-17-tlb-management/) | 缓存/TLB · 选读（可对照 [03 Hennessy](../../03-Computer-Architecture-6th/)） |
| 18–20 | [ch18](./chapter-18-memory-barriers/) … [ch20](./chapter-20-atomic-operations/) | 屏障/原子 · 精读 |
| 21 | [chapter-21-os-topics](./chapter-21-os-topics/) | OS 话题 · 精读 |
| 22–23 | [ch22](./chapter-22-fp-neon/) · [ch23](./chapter-23-sve-optimization/) | NEON/SVE · 选读/跳过 |

← [19 模块总览](../README.md)
