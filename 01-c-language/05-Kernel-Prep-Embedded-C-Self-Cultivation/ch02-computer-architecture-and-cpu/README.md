# 第 2 章 计算机体系结构与 CPU 工作原理

**Computer Architecture and CPU**

## 本章目标

建立 **从硅片到指令** 的完整硬件图景：芯片制造、CPU 设计、冯/哈佛体系结构、Cache 与流水线、多核与异构、总线编址与 ISA——能解释 C 语句在 CPU/内存/外设上的行为，为 ARM 汇编、内核/DPDK 编程打底。

## 前置依赖

**[第 1 章 ch01](../ch01-tools-of-the-trade/)** —— 会用 `gcc`、`gdb`、`objdump`、`make`；能读基础反汇编。

## 快速运行 Demo

```bash
cd 00-Linux-Kernel-DPDK-Network-C/05-Kernel-Prep-Embedded-C-Self-Cultivation/ch02-computer-architecture-and-cpu/demo
make all && ./demo01_endian
gdb ./demo01_endian
# (gdb) break main
# (gdb) run
# (gdb) x/4xb &v    # 验证大小端
make clean
```

## 六大知识模块

| 模块 | 目录 | 核心 |
|------|------|------|
| **1 芯片与 CPU 设计** | 2.1–2.2 | 硅→PN 结→流片；图灵机→RTL |
| **2 体系结构** | 2.3 | 冯·诺依曼 / 哈佛 / 混合 |
| **3 Cache** | 2.4 | 局部性、L1/L3、DMA 一致性 |
| **4 流水线** | 2.5 | 冒险、分支预测、SIMD |
| **5 多核与异构** | 2.6–2.7 | 互连、绑核、GPU/FPGA/DPU |
| **6 总线与 ISA** | 2.8–2.9 | MMIO、端序、汇编寻址 |

## Demo 清单

| Demo | 内容 | 对应小节 |
|------|------|----------|
| **demo01_endian** | 大小端检测 + gdb 看内存 | **2.8.4** |
| ch01 Demo03 | `objdump -d` 看 cmp/branch | **2.2.2**、**2.9.3** |

## 考核要点

1. 画出 **冯·诺依曼五部件** 与一条 `a=b+c` 的数据流  
2. 解释 **Cache 行、伪共享**，说出 DPDK 对齐 padding 的原因  
3. 区分 **结构/数据/控制冒险**；说明 `likely/unlikely` 的作用  
4. 口述 **MMIO**：`volatile`、设备寄存器 vs RAM  
5. 检测 **大小端** 并说明网络字节序转换  
6. 列举 **ISA 与微架构** 区别；读懂基础 **寻址方式** 汇编

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | **ch01** 工具链 |
| 后置 | **ch03** ARM 汇编；**ch04** 编译链接；**ch05** 内存堆栈 |

## 小节

- [2.1 一颗芯片是怎样诞生的](./2.1-chip/2.1-一颗芯片是怎样诞生的.md)
  - [2.1.1 从沙子到单晶硅](./2.1-chip/2.1.1-从沙子到单晶硅.md)
  - [2.1.2 PN结的工作原理](./2.1-chip/2.1.2-PN结的工作原理.md)
  - [2.1.3 从PN结到芯片电路](./2.1-chip/2.1.3-从PN结到芯片电路.md)
  - [2.1.4 芯片的封装](./2.1-chip/2.1.4-芯片的封装.md)
- [2.2 一颗CPU是怎么设计出来的](./2.2-cpu-design/2.2-一颗CPU是怎么设计出来的.md)
  - [2.2.1 计算机理论基石：图灵机](./2.2-cpu-design/2.2.1-计算机理论基石-图灵机.md)
  - [2.2.2 CPU内部结构及工作原理](./2.2-cpu-design/2.2.2-CPU内部结构及工作原理.md)
  - [2.2.3 CPU设计流程](./2.2-cpu-design/2.2.3-CPU设计流程.md)
- [2.3 计算机体系结构](./2.3-architecture/2.3-计算机体系结构.md)
  - [2.3.1 冯·诺依曼架构](./2.3-architecture/2.3.1-冯-诺依曼架构.md)
  - [2.3.2 哈弗架构](./2.3-architecture/2.3.2-哈弗架构.md)
  - [2.3.3 混合架构](./2.3-architecture/2.3.3-混合架构.md)
- [2.4 CPU性能提升：Cache机制](./2.4-cache/2.4-CPU性能提升-Cache机制.md)
  - [2.4.1 Cache的工作原理](./2.4-cache/2.4.1-Cache的工作原理.md)
  - [2.4.2 一级Cache和二级Cache](./2.4-cache/2.4.2-一级Cache和二级Cache.md)
  - [2.4.3 为什么有些处理器没有Cache](./2.4-cache/2.4.3-为什么有些处理器没有Cache.md)
- [2.5 CPU性能提升：流水线](./2.5-pipeline/2.5-CPU性能提升-流水线.md)
  - [2.5.1 流水线工作原理](./2.5-pipeline/2.5.1-流水线工作原理.md)
  - [2.5.2 超流水线技术](./2.5-pipeline/2.5.2-超流水线技术.md)
  - [2.5.3 流水线冒险](./2.5-pipeline/2.5.3-流水线冒险.md)
  - [2.5.4 分支预测](./2.5-pipeline/2.5.4-分支预测.md)
  - [2.5.5 乱序执行](./2.5-pipeline/2.5.5-乱序执行.md)
  - [2.5.6 SIMD和NEON](./2.5-pipeline/2.5.6-SIMD和NEON.md)
  - [2.5.7 单发射和多发射](./2.5-pipeline/2.5.7-单发射和多发射.md)
- [2.6 多核CPU](./2.6-multicore/2.6-多核CPU.md)
  - [2.6.1 单核处理器的瓶颈](./2.6-multicore/2.6.1-单核处理器的瓶颈.md)
  - [2.6.2 片上多核互连技术](./2.6-multicore/2.6.2-片上多核互连技术.md)
  - [2.6.3 big.LITTLE结构](./2.6-multicore/2.6.3-big-LITTLE结构.md)
  - [2.6.4 超线程技术](./2.6-multicore/2.6.4-超线程技术.md)
  - [2.6.5 CPU核数越多越好吗](./2.6-multicore/2.6.5-CPU核数越多越好吗.md)
- [2.7 后摩尔时代：异构计算的崛起](./2.7-heterogeneous/2.7-后摩尔时代-异构计算的崛起.md)
  - [2.7.1 什么是异构计算](./2.7-heterogeneous/2.7.1-什么是异构计算.md)
  - [2.7.2 GPU](./2.7-heterogeneous/2.7.2-GPU.md)
  - [2.7.3 DSP](./2.7-heterogeneous/2.7.3-DSP.md)
  - [2.7.4 FPGA](./2.7-heterogeneous/2.7.4-FPGA.md)
  - [2.7.5 TPU](./2.7-heterogeneous/2.7.5-TPU.md)
  - [2.7.6 NPU](./2.7-heterogeneous/2.7.6-NPU.md)
  - [2.7.7 后摩尔时代的XPU们](./2.7-heterogeneous/2.7.7-后摩尔时代的XPU们.md)
- [2.8 总线与地址](./2.8-bus/2.8-总线与地址.md)
  - [2.8.1 地址的本质](./2.8-bus/2.8.1-地址的本质.md)
  - [2.8.2 总线的概念](./2.8-bus/2.8.2-总线的概念.md)
  - [2.8.3 总线编址方式](./2.8-bus/2.8.3-总线编址方式.md)
  - [2.8.4 大小端字节序](./2.8-bus/2.8.4-大小端字节序.md)
- [2.9 指令集与微架构](./2.9-isa/2.9-指令集与微架构.md)
  - [2.9.1 什么是指令集](./2.9-isa/2.9.1-什么是指令集.md)
  - [2.9.2 什么是微架构](./2.9-isa/2.9.2-什么是微架构.md)
  - [2.9.3 指令助记符：汇编语言](./2.9-isa/2.9.3-指令助记符-汇编语言.md)


---

## 代码自测

**题目 1：** 以下代码在 32 位 ARM 和 64 位 x86 上运行结果不同，为什么？
```c
#include <stdio.h>
int main(void) {
    int *p = 0;
    printf("sizeof(p) = %zu\n", sizeof(p));
    printf("sizeof(int) = %zu\n", sizeof(int));
    // 在 32 位 ARM：sizeof(p)=4, sizeof(int)=4
    // 在 64 位 x86：sizeof(p)=8, sizeof(int)=4
    return 0;
}
```
<details>
<summary>参考答案</summary>

指针大小取决于 CPU 架构和地址总线宽度：
- 32 位 ARM：指针 4 字节（32 位地址空间）
- 64 位 x86：指针 8 字节（64 位地址空间）

int 大小通常固定为 4 字节（但标准只保证 sizeof(short) <= sizeof(int) <= sizeof(long)）。

这体现了 CPU 架构对 C 程序的影响：数据类型大小、字节序（大端/小端）、内存对齐要求都依赖架构。跨平台代码应使用 stdint.h 中的固定宽度类型（uint32_t、int64_t）而不是 int/long。这是 ch02 计算机架构与 CPU 的核心知识点。

</details>

## 代码自测

**题目 1：** 嵌入式 C 自我修养这本书的学习路线是什么？为什么说它是标准 C 到内核的桥梁？

<details>
<summary>参考答案</summary>

路线：标准 C → GNU C 扩展 → 嵌入式系统编程 → 操作系统基础。桥梁作用：内核代码大量使用 __attribute__/typeof/container_of/section/weak 等 GNU 扩展，这些标准 C 教材不讲。本书填补了标准 C 到内核驱动开发之间的知识空白。

</details>
