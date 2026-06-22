# Computer Systems: A Programmer's Perspective 3rd — Bryant & O'Neill

**文件夹 01** · 全书 **12 章 + 附录 A** · [返回总清单](../READING-LIST.md#6-computer-systems-a-programmers-perspective-3rd--bryant--oneill)

> **文件夹 01** · 知其所以然 — 程序如何在硬件上跑。  
> **下一本：** [02-SysPerf](../02-Systems-Performance-2nd/) → [03-BPF](../03-BPF-Performance-Tools/) → [11-HFT](../11-HFT-Low-Latency-Practice/) / [12-Rust](../12-Rust-Quant-Trading-Guide/)  
> 全链路 → [LEARNING-CHAIN.md](../LEARNING-CHAIN.md)

📋 **完整目录与 HFT 读/跳标注** → [OUTLINE.md](./OUTLINE.md)

---

## 全书结构

```
chapter-XX-english-slug/   ← Ch 1 已采用；其余章仍为单文件，后续可迁移
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

→ 读完地基再读 [02-SysPerf](../02-Systems-Performance-2nd/) · Hennessy 理论 → [04-Computer-Architecture-6th](../04-Computer-Architecture-6th/)
