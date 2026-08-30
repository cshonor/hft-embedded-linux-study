# Learning eBPF · 第 1 章：什么是 eBPF，它为何重要

> **原书：** Chapter 1: What Is eBPF and Why Is It Important?  
> **HFT：** 🟡 · **底本：** LEARNING-EBPF-BILINGUAL.pdf（GPT 双语逐段对照；PDF 存仓库外 `~/Desktop/hft-local-books/`，不入库）

> 底本：LEARNING-EBPF-BILINGUAL.pdf（Liz Rice, O'Reilly 2023，GPT 双语版）
> 本章是全书的地基章：讲清 eBPF 的来历、它与"改内核/写模块"两条老路的本质区别。

## 本章目标

1. 说清 eBPF 的定义：**可动态加载进内核、改变内核行为的自定义代码**
2. 从 BPF → eBPF 的演进时间线
3. 为什么传统路径（改内核源码、写内核模块）不能快速安全地扩展内核
4. eBPF 的三大用例：观测（observability）、网络、安全

## 小节索引

| 原书小节 | 笔记 |
|---|---|
| §1.1–1.3 | [1.1 起源与演进](./notes/1.1_起源与演进.md) |
| §1.4–1.5 | [1.2 内核边界与传统扩展路径](./notes/1.2_内核边界与传统扩展路径.md) |
| §1.6–1.7 | [1.3 三大优势与云原生对比](./notes/1.3_三大优势与云原生对比.md) |
| §1.8–1.9 | [1.4 HFT关联与自测题](./notes/1.4_HFT关联与自测题.md) |

## 交叉引用

- 验证器细节 → `../chapter-06-verifier/`
- bpf() 系统调用 → `../chapter-04-bpf-syscall/`
- XDP 数据路径 → `../chapter-08-networking/`
- BPF 之巅对照：`../EBPF-BOOKS-COMPARISON.md`
