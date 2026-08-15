# Learning eBPF · 第 1 章：什么是 eBPF，它为何重要

> **原书：** Chapter 1: What Is eBPF and Why Is It Important?  
> **HFT：** 🟡 · **底本：** [LEARNING-EBPF-BILINGUAL.pdf](../LEARNING-EBPF-BILINGUAL.pdf)（GPT 双语逐段对照）

> 底本：LEARNING-EBPF-BILINGUAL.pdf（Liz Rice, O'Reilly 2023，GPT 双语版）
> 本章是全书的地基章：讲清 eBPF 的来历、它与"改内核/写模块"两条老路的本质区别。

## 本章目标

1. 说清 eBPF 的定义：**可动态加载进内核、改变内核行为的自定义代码**
2. 从 BPF → eBPF 的演进时间线
3. 为什么传统路径（改内核源码、写内核模块）不能快速安全地扩展内核
4. eBPF 的三大用例：观测（observability）、网络、安全

## 小节索引

| 节 | 笔记 |
|----|------|
| 1. 起源：1993 年的 BSD Packet Filter 论文 | [notes/section-1-起源：1993年的BSDPacketFilter论文.md](./notes/section-1-起源：1993年的BSDPacketFilter论文.md) |
| 2. 演进时间线（记住这几个节点） | [notes/section-2-演进时间线（记住这几个节点）.md](./notes/section-2-演进时间线（记住这几个节点）.md) |
| 3. eBPF 与 BPF 的称呼 | [notes/section-3-eBPF与BPF的称呼.md](./notes/section-3-eBPF与BPF的称呼.md) |
| 4. 内核与用户空间（底层视角） | [notes/section-4-内核与用户空间（底层视角）.md](./notes/section-4-内核与用户空间（底层视角）.md) |
| 5. 传统扩展内核的两条路（以及为什么不行） | [notes/section-5-传统扩展内核的两条路（以及为什么不行）.md](./notes/section-5-传统扩展内核的两条路（以及为什么不行）.md) |
| 6. eBPF 的三大优势 | [notes/section-6-eBPF的三大优势.md](./notes/section-6-eBPF的三大优势.md) |
| 7. 云原生环境：对比 sidecar 模型 | [notes/section-7-云原生环境：对比sidecar模型.md](./notes/section-7-云原生环境：对比sidecar模型.md) |
| HFT 关联 | [notes/section-8-HFT关联.md](./notes/section-8-HFT关联.md) |
| 自测题 | [notes/section-9-自测题.md](./notes/section-9-自测题.md) |

## 交叉引用

- 验证器细节 → `../chapter-06-verifier/`
- bpf() 系统调用 → `../chapter-04-bpf-syscall/`
- XDP 数据路径 → `../chapter-08-networking/`
- BPF 之巅对照：`../EBPF-BOOKS-COMPARISON.md`
