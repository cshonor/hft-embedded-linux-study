# Learning eBPF · 第 2 章：eBPF 的 "Hello World"

> **原书：** Chapter 2: eBPF's "Hello World"  
> **HFT：** 🔴 · **底本：** [LEARNING-EBPF-BILINGUAL.pdf](../LEARNING-EBPF-BILINGUAL.pdf)（GPT 双语逐段对照）

> 本章用 BCC Python 框架写三个渐进的 Hello World，引出 helper 函数、maps、perf/ring buffer、尾调用四大构件。

## 本章目标

1. 建立心智模型：**用户态程序（加载/读数据） + 内核态 eBPF 程序（事件触发执行）**
2. 掌握三种数据出口：trace_pipe（调试用）→ hash map（轮询）→ perf/ring buffer（事件推送）
3. 理解函数调用限制与尾调用机制

## 小节索引

| 节 | 笔记 |
|----|------|
| 1. 第一个 Hello World（BCC 版） | [notes/section-1-第一个HelloWorld（BCC版）.md](./notes/section-1-第一个HelloWorld（BCC版）.md) |
| 2. BPF Maps：结构化数据通道 | [notes/section-2-BPFMaps：结构化数据通道.md](./notes/section-2-BPFMaps：结构化数据通道.md) |
| 3. Perf / Ring Buffer：事件推送 | [notes/section-3-Perf／RingBuffer：事件推送.md](./notes/section-3-Perf／RingBuffer：事件推送.md) |
| 4. 函数调用 | [notes/section-4-函数调用.md](./notes/section-4-函数调用.md) |
| 5. 尾调用（Tail Calls） | [notes/section-5-尾调用（TailCalls）.md](./notes/section-5-尾调用（TailCalls）.md) |
| 坑点清单 | [notes/section-6-坑点清单.md](./notes/section-6-坑点清单.md) |
| HFT 关联 | [notes/section-7-HFT关联.md](./notes/section-7-HFT关联.md) |
| 自测题 | [notes/section-8-自测题.md](./notes/section-8-自测题.md) |

## 交叉引用

- 编译/JIT/BPF 子程序 → `../chapter-03-anatomy-of-ebpf-program/`
- ring buffer 的 bpf() 侧操作 → `../chapter-04-bpf-syscall/`
- CO-RE 与 libbpf（BCC 的替代） → `../chapter-05-core-btf-libbpf/`
- 程序类型与可用 helper → `../chapter-07-program-attachment-types/`
