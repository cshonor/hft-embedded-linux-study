# 第 2 章 计算机体系结构与 CPU 工作原理

**Computer Architecture and CPU**

## 本章目标

建立 **从硅片到指令** 的完整硬件图景：芯片制造、CPU 设计、冯/哈佛体系结构、Cache 与流水线、多核与异构、总线编址与 ISA——能解释 C 语句在 CPU/内存/外设上的行为，为 ARM 汇编、内核/DPDK 编程打底。

## 前置依赖

**[第 1 章 ch01](../ch01-tools-of-the-trade/)** —— 会用 `gcc`、`gdb`、`objdump`、`make`；能读基础反汇编。

## 快速运行 Demo

```bash
cd 00-Linux-Kernel-DPDK-Network-C/04-Kernel-Prep-Embedded-C-Self-Cultivation/ch02-computer-architecture-and-cpu/demo
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

## 章节自测

> 理解 CPU 才能写出高性能 C。看代码 → 想答案 → 点开验证。

### Q1: Cache 层次

```
// 一次内存访问的延迟：
L1 cache: ~1ns
L2 cache: ~4ns
L3 cache: ~12ns
DRAM:     ~100ns

// HFT 为什么对 cache 疯狂？
```

<details>
<summary>答案与复习指引</summary>

**答案：** L1 和 DRAM 差 100 倍。HFT 每纳秒都关键——一个 cache miss = 100ns 延迟 = 在 100ns 内对手可能已完成交易。

**HFT 优化：**
- 数据紧凑（`packed` 减小 footprint）
- 数据局部性（循环访问连续内存）
- 预取（`__builtin_prefetch`）
- 缓存行对齐（`aligned(64)`）
- 避免伪共享

**复习：** → [2.5 Cache 与流水线](./2.5-cache-pipeline/2.5-Cache与流水线.md)

</details>

### Q2: 流水线分支预测

```c
// 写法 A：分支
if (data[i] > threshold)
    sum += data[i];

// 写法 B：无分支
sum += (data[i] > threshold) ? data[i] : 0;
```

> 两种写法在数据随机时性能差多少？为什么？

<details>
<summary>答案与复习指引</summary>

**答案：** 数据随机时写法 A 可能慢 2-5 倍——CPU 分支预测失败后需**冲刷流水线**（~15-20 周期）。写法 B 无分支，用 `cmov` 或算术替代，无预测失败惩罚。

**但：** 数据有规律时（如几乎都是 true），分支预测命中率高，写法 A 可能更快（条件赋值比无条件计算快）。

**HFT 实践：** 热路径上数据随机时用无分支写法。`__builtin_expect` 给编译器提示分支倾向。

**复习：** → [2.5 Cache 与流水线](./2.5-cache-pipeline/2.5-Cache与流水线.md)

</details>

### Q3: 大小端检测

```c
int x = 0x12345678;
char *p = (char*)&x;

printf("%02x\n", p[0]);  // 输出什么？
// 小端机和大端机分别输出什么？
```

<details>
<summary>答案与复习指引</summary>

**答案：**
- 小端（x86/ARM 默认）：`p[0] = 0x78`（低字节存低地址）
- 大端（网络字节序/PowerPC）：`p[0] = 0x12`（高字节存低地址）

**网络协议**用大端（`htonl`/`ntohs` 转换）。x86/ARM 默认小端。跨平台传输二进制数据必须处理字节序。

**复习：** → [2.8 总线编址](./2.8-bus-addressing/2.8-总线编址.md) — 大小端
