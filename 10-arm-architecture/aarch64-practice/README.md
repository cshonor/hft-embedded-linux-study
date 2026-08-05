# 《ARM64体系结构编程与实践》

> **奔跑吧Linux社区** · 人民邮电出版社 · **模块 19 · AArch64 实战主书**  
> **平台：** 原书 **Pi4B** + QEMU；**本仓库实机 = Pi5** — 见 [PI5-ADAPT.md](./PI5-ADAPT.md)（架构实验主力 QEMU `-cpu cortex-a76`）  
> **实验代码：** [github.com/runninglinuxkernel/arm64_programming_practice](https://github.com/runninglinuxkernel/arm64_programming_practice)

---

## 定位

| | |
|---|---|
| **补什么** | **ARMv8/v9 · A64 64 位指令** · 异常/中断 · **GIC** · **内存管理** |
| **与 arm32-asm 关系** | [Hohl/Hinds v4T/v7-M](../arm32-asm/) = 汇编思维入门（**讲 A、练 M**）；**本书 = AArch64 / A64 主战场**（Pi5） |
| **飞控/无人机** | 异常 · GIC · MM · 屏障/原子 — 对接 [21 驱动](../../12-device-drivers-dt/) · [23 飞控](../../14-motion-control/) |
| **下一步** | [20 U-Boot/构建](../../11-embedded-boot-build/) |

📚 **全书总结** → [BOOK-SUMMARY.md](./BOOK-SUMMARY.md)  
🔧 **Pi5 适配** → [PI5-ADAPT.md](./PI5-ADAPT.md)（vs 原书 Pi4B）  
📋 **章节目录与裁剪** → [OUTLINE.md](./OUTLINE.md)  
📛 **名词：** [AArch64 vs ARM64 vs A64](./AARCH64-NAMING.md)（为何不笼统叫「ARM」）

---

## 章节目录（Ch 1–23）

| 章 | 文件夹 | 标签 |
|----|--------|------|
| 1 | [chapter-01-arm64-fundamentals](./chapter-01-arm64-fundamentals/) | 精读 |
| 2 | [chapter-02-raspberry-pi-lab](./chapter-02-raspberry-pi-lab/) | 精读 |
| 3–7 | [ch03](./chapter-03-a64-load-store/) … [ch07](./chapter-07-a64-traps/) | **A64 指令集** · 精读 |
| 8–10 | [ch08](./chapter-08-gnu-assembler/) … [ch10](./chapter-10-gcc-inline-asm/) | 工具链 · 精读 |
| 11–14 | [ch11](./chapter-11-exception-handling/) … [ch14](./chapter-14-memory-management/) | **异常/GIC/MM** · 精读 |
| 15–17 | [ch15](./chapter-15-cache-basics/) … [ch17](./chapter-17-tlb-management/) | 缓存/TLB · 选读（可对照 [03 Hennessy](../../03-computer-architecture/)） |
| 18–20 | [ch18](./chapter-18-memory-barriers/) … [ch20](./chapter-20-atomic-operations/) | 屏障/原子 · 精读 |
| 21 | [chapter-21-os-topics](./chapter-21-os-topics/) | OS 话题 · 精读 |
| 22–23 | [ch22](./chapter-22-fp-neon/) · [ch23](./chapter-23-sve-optimization/) | NEON/SVE · 选读/跳过 |

← [19 模块总览](../README.md)
