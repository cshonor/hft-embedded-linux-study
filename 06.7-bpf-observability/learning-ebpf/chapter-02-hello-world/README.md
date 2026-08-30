# Learning eBPF · 第 2 章：eBPF 的 "Hello World"

> **原书：** Chapter 2: eBPF's "Hello World"  
> **HFT：** 🔴 · **底本：** LEARNING-EBPF-BILINGUAL.pdf（GPT 双语逐段对照；PDF 存仓库外 `~/Desktop/hft-local-books/`，不入库）

> 本章用 BCC Python 框架写三个渐进的 Hello World，引出 helper 函数、maps、perf/ring buffer、尾调用四大构件。

## 本章目标

1. 建立心智模型：**用户态程序（加载/读数据） + 内核态 eBPF 程序（事件触发执行）**
2. 掌握三种数据出口：trace_pipe（调试用）→ hash map（轮询）→ perf/ring buffer（事件推送）
3. 理解函数调用限制与尾调用机制

## 小节索引

| 原书小节 | 笔记 |
|---|---|
| §2.1–2.3 | [2.1 HelloWorld与数据通道](./notes/2.1_HelloWorld与数据通道.md) |
| §2.4–2.5 | [2.2 函数调用与尾调用](./notes/2.2_函数调用与尾调用.md) |
| §2.6–2.8 | [2.3 坑点HFT关联与自测](./notes/2.3_坑点HFT关联与自测.md) |

## 交叉引用

- 编译/JIT/BPF 子程序 → `../chapter-03-anatomy-of-ebpf-program/`
- ring buffer 的 bpf() 侧操作 → `../chapter-04-bpf-syscall/`
- CO-RE 与 libbpf（BCC 的替代） → `../chapter-05-core-btf-libbpf/`
- 程序类型与可用 helper → `../chapter-07-program-attachment-types/`
