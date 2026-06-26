# Computer Architecture 6th — Hennessy & Patterson

**文件夹 07** · 全书 **7 章 + 附录 A–M**（含在线附录）· [返回总清单](../READING-LIST.md#5-computer-architecture-a-quantitative-approach-6th--hennessy--patterson)

> **HFT 阶段 1：** 先读 **Ch2**（配合 [01-CSAPP Ch6](../01-CSAPP-3rd/chapter-06-memory-hierarchy/)），再读 [03-SysPerf](../03-Systems-Performance-2nd/)。Ch5 内存一致性可与 CSAPP Ch12 交叉。

📋 **完整目录与 HFT 读/跳标注** → [OUTLINE.md](./OUTLINE.md)

---

## 正文章节（7 章）

| 章 | 笔记 |
|----|------|
| 1 量化设计与分析基础 | [chapter-01-量化设计与分析基础.md](./chapter-01-量化设计与分析基础.md) |
| 2 存储器层次结构设计 | [chapter-02-存储器层次结构设计.md](./chapter-02-存储器层次结构设计.md) |
| 3 指令级并行 | [chapter-03-指令级并行.md](./chapter-03-指令级并行.md) |
| 4 向量/SIMD/GPU | [chapter-04-向量SIMD与GPU.md](./chapter-04-向量SIMD与GPU.md) |
| 5 线程级并行 | [chapter-05-线程级并行.md](./chapter-05-线程级并行.md) |
| 6 仓储级计算机 | [chapter-06-仓储级计算机.md](./chapter-06-仓储级计算机.md) |
| 7 特定领域架构 | [chapter-07-特定领域架构.md](./chapter-07-特定领域架构.md) |

## 书内附录（A–C）

| | 笔记 |
|---|------|
| A 指令集原理 | [appendix-A-指令集原理.md](./appendix-A-指令集原理.md) |
| B 存储层次复习 | [appendix-B-存储层次复习.md](./appendix-B-存储层次复习.md) |
| C 流水线 | [appendix-C-流水线.md](./appendix-C-流水线.md) |

## 在线附录（D–M）

| | 笔记 |
|---|------|
| D 存储系统 | [appendix-D-存储系统.md](./appendix-D-存储系统.md) |
| E 嵌入式系统 | [appendix-E-嵌入式系统.md](./appendix-E-嵌入式系统.md) |
| F 互连网络 | [appendix-F-互连网络.md](./appendix-F-互连网络.md) |
| G 深入向量处理器 | [appendix-G-深入向量处理器.md](./appendix-G-深入向量处理器.md) |
| H VLIW 与 EPIC | [appendix-H-VLIW与EPIC.md](./appendix-H-VLIW与EPIC.md) |
| I 大规模多处理器 | [appendix-I-大规模多处理器.md](./appendix-I-大规模多处理器.md) |
| J 计算机算术 | [appendix-J-计算机算术.md](./appendix-J-计算机算术.md) |
| K 指令集架构综述 | [appendix-K-指令集架构综述.md](./appendix-K-指令集架构综述.md) |
| L 地址转换高级概念 | [appendix-L-地址转换高级概念.md](./appendix-L-地址转换高级概念.md) |
| M 历史视角与参考文献 | [appendix-M-历史视角与参考文献.md](./appendix-M-历史视角与参考文献.md) |

---

## HFT 精读捷径

```
Ch 2 → Ch 5 → Ch 1 → Ch 3
附录 B/C（交叉复习）
```

**HFT 产出：** 无锁队列、订单簿布局、cache-line padding 的硬件依据。

## 交叉阅读

- 程序员落地 → [01-CSAPP-3rd](../01-CSAPP-3rd/)
- 虚拟内存 → [03-Gorman](../07-Linux-Virtual-Memory-Manager/)
- 跨模块 → [CROSS-MODULE-GUIDE.md](../CROSS-MODULE-GUIDE.md)
