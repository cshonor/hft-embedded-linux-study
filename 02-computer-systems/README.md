# Computer Systems: A Programmer's Perspective 3rd — Bryant & O'Neill

**文件夹 02** · 全书 **12 章 + 附录 A** · [返回总清单](../READING-LIST.md#6-computer-systems-a-programmers-perspective-3rd--bryant--oneill)

> **文件夹 02** · 知其所以然 — 程序如何在硬件上跑。  
> **机器级默认架构：** **x86-64 + Linux System V + AT&T gas**（Ch3 起）；**HFT 只练 AT&T，不学 Intel 语法**。Ch4 **Y86-64** 仅为流水线教学子集。ARM 对照 → [07-ARM64](../07-arm-architecture/)。
> **下一本：** [16-Systems-Performance](../16-systems-performance/) → [17-BPF](../17-bpf-observability/) → [18-HFT](../18-hft-engineering/) / [21-Rust](../21-rust-quant/)  
> 全链路 → [README.md](../README.md)

📋 **完整目录与 HFT 读/跳标注** → [OUTLINE.md](./OUTLINE.md)

---

## 全书结构

```
chapter-XX-english-slug/   ← 全书 12 章均已采用
├── README.md
└── notes/section-*.md
```

### 第 1 章
| 章 | 笔记 |
|----|------|
| 1 计算机系统漫游 | [chapter-01-tour-of-computer-systems](./chapter-01-tour-of-computer-systems/) |

### Part I · 程序结构和执行
| 章 | 笔记 |
|----|------|
| 2 信息的表示和处理 | [chapter-02-representing-information](./chapter-02-representing-information/) |
| 3 程序的机器级表示 | [chapter-03-machine-level-programs](./chapter-03-machine-level-programs/) |
| 4 处理器体系结构 | [chapter-04-processor-architecture](./chapter-04-processor-architecture/) |
| 5 优化程序性能 | [chapter-05-optimizing-performance](./chapter-05-optimizing-performance/) |
| 6 存储器层次结构 | [chapter-06-memory-hierarchy/](./chapter-06-memory-hierarchy/) |

### Part II · 在系统上运行程序
| 章 | 笔记 |
|----|------|
| 7 链接 | [chapter-07-linking/](./chapter-07-linking/) |
| 8 异常控制流 | [chapter-08-exceptional-control-flow/](./chapter-08-exceptional-control-flow/) |
| 9 虚拟内存 | [chapter-09-virtual-memory/](./chapter-09-virtual-memory/) |

### Part III · 程序间交互和通信
| 章 | 笔记 |
|----|------|
| 10 系统级 I/O | [chapter-10-system-io/](./chapter-10-system-io/) |
| 11 网络编程 | [chapter-11-network-programming/](./chapter-11-network-programming/) |
| 12 并发编程 | [chapter-12-concurrent-programming/](./chapter-12-concurrent-programming/) |

### 附录
| | 笔记 |
|---|------|
| A 错误处理 | [appendix-A-错误处理.md](./appendix-A-错误处理.md) |

---

## HFT 精读捷径

### ① 地基篇（SysPerf 之前 · 与 Hennessy Ch2 交叉）

```
Hennessy Ch2（理论）→ CSAPP Ch6（落地）
→ Ch8 进程/syscall → Ch9 VM → Ch12 锁与并发
选读 Ch1 概览 · Ch4 流水线 · 时间紧可 Ch5 后移
```

### ② 网络篇（阶段 5 · UNP 前后）

```
Ch 10–11 网络 / epoll
```

→ 读完地基再读 [16-Systems-Performance](../16-systems-performance/) · Hennessy 理论 → [19-computer-architecture](../19-computer-architecture/)

---

## x86-64 汇编学习路径（与 ARM64 的关系）

> **结论：不需要单独学 x86 汇编书。CSAPP Ch3 就是完整的 x86-64 汇编课。**

```
ARM64 汇编（07）         CSAPP Ch3（02）           HFT 平台深水区（15-19）
━━━━━━━━━━━━━━         ━━━━━━━━━━━━━━            ━━━━━━━━━━━━━━━━━━
先学 ARM64 打基础    →   自带 x86-64 汇编      →   Intel SDM / Optimization Manual
概念全一样，换方言        Ch3 覆盖读写能力           cache / NUMA / RDTSC / pipeline
```

| 阶段 | 做什么 | 为什么 |
|------|--------|--------|
| **1. ARM64 先行**（[07](../07-arm-architecture/)） | 学透 AArch64 汇编 | 最干净的现代 ISA：定长编码、规整寄存器、无历史包袱，适合做第一门汇编 |
| **2. CSAPP Ch3**（本模块） | x86-64 汇编读写 | 已有 ARM64 底子，概念全一样（寄存器/条件码/调用约定/栈帧），只是语法和编码不同 |
| **3. HFT 平台特性**（[15](../15-dpdk/)–[19](../19-computer-architecture/)） | x86 平台性能优化 | 真正花时间的不是汇编语法，而是 cache 层次/TLB/NUMA/RDTSC/pipeline stall |

**不单独学 x86 汇编书的理由：**

- x86 是 CISC，变长编码 + 大量遗留指令，单独学习效率低
- CSAPP Ch3 覆盖已足够"读得懂、写得对"
- HFT 真正需要的是平台特性（cache/NUMA/pipeline），在 CSAPP Ch5-9 有基础覆盖，更深入看 Intel SDM（Software Developer's Manual）和 Intel Optimization Manual
