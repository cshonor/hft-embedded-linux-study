# Hennessy 6th — 全书目录（7 章 + 附录 A–M）

> **Computer Architecture: A Quantitative Approach, 6th Edition** · Hennessy & Patterson

| 标签 | HFT 读法 |
|------|----------|
| 🔴 | 精读 |
| 🟡 | 选读 |
| ⚪ | 跳过 |

## 正文章节

| 章 | 英文 | 笔记 | HFT |
|----|------|------|-----|
| 1 | Fundamentals of Quantitative Design and Analysis | [chapter-01](./chapter-01-quantitative-design-fundamentals/) | 🟡 |
| 2 | Memory Hierarchy Design | [chapter-02](./chapter-02-memory-hierarchy-design/) | 🔴 |
| 3 | Instruction-Level Parallelism | [chapter-03](./chapter-03-instruction-level-parallelism/) | 🟡 |
| 4 | Vector, SIMD, and GPU | [chapter-04](./chapter-04-vector-simd-gpu/) | ⚪ |
| 5 | Thread-Level Parallelism | [chapter-05](./chapter-05-thread-level-parallelism/) | 🔴 |
| 6 | Warehouse-Scale Computers | [chapter-06](./chapter-06-warehouse-scale-computers/) | ⚪ |
| 7 | Domain-Specific Architectures | [chapter-07](./chapter-07-domain-specific-architectures/) | ⚪ |

## 书内附录

| | 英文 | 笔记 | HFT |
|---|------|------|-----|
| A | Instruction Set Principles | [appendix-A](./appendix-A-指令集原理.md) | 🟡 |
| B | Review of Memory Hierarchy | [appendix-B](./appendix-B-存储层次复习.md) | 🟡 |
| C | Pipelining | [appendix-C](./appendix-C-流水线.md) | 🟡 |

## 在线附录（Online Appendices）

| | 英文 | 笔记 | HFT |
|---|------|------|-----|
| D | Storage Systems | [appendix-D](./appendix-D-存储系统.md) | ⚪ |
| E | Embedded Systems | [appendix-E](./appendix-E-嵌入式系统.md) | ⚪ |
| F | Interconnection Networks | [appendix-F](./appendix-F-互连网络.md) | 🟡 |
| G | Vector Processors in More Depth | [appendix-G](./appendix-G-深入向量处理器.md) | ⚪ |
| H | VLIW and EPIC | [appendix-H](./appendix-H-VLIW与EPIC.md) | ⚪ |
| I | Large-Scale Multiprocessors | [appendix-I](./appendix-I-大规模多处理器.md) | ⚪ |
| J | Computer Arithmetic | [appendix-J](./appendix-J-计算机算术.md) | ⚪ |
| K | Survey of ISAs | [appendix-K](./appendix-K-指令集架构综述.md) | ⚪ |
| L | Advanced Address Translation | [appendix-L](./appendix-L-地址转换高级概念.md) | 🟡 |
| M | Historical Perspectives | [appendix-M](./appendix-M-历史视角与参考文献.md) | ⚪ |

> 词汇表 / 参考文献 / 索引：不单独建笔记文件。

---

## HFT 精读顺序

```
Ch 2  Cache line、MESI、false sharing、NUMA
Ch 5  内存一致性、store buffer、memory order
Ch 1  Roofline（性能上限直觉）
Ch 3  ILP、分支预测（热循环微优化）
附录 B/C（与 Ch2、CSAPP Ch4 交叉）
```

→ 程序员落地 → [01-CSAPP-3rd](../01-CSAPP-3rd/)  
→ 虚拟内存衔接 → [03-Gorman](../05-Linux-Virtual-Memory-Manager/)

完整路线 → [HFT-READING-ROADMAP.md](../HFT-READING-ROADMAP.md)
