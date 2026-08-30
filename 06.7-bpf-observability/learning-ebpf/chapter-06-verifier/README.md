# Learning eBPF · 第 6 章：eBPF 验证器

> **原书：** Chapter 6: The eBPF Verifier  
> **HFT：** 🔴 · **底本：** LEARNING-EBPF-BILINGUAL.pdf（GPT 双语逐段对照；PDF 存仓库外 `~/Desktop/hft-local-books/`，不入库）

> 验证器是 eBPF 安全模型的基石：加载前穷举所有执行路径证明程序无害。本章大量篇幅是"故意写坏代码 → 看验证器怎么骂你"，是日后排查 `invalid mem access` 类报错的实操手册。

## 本章目标

1. 理解验证算法：逐指令模拟 + 寄存器状态跟踪 + 分支状态栈 + 剪枝
2. 能读懂验证器日志（指令、寄存器状态、值域信息）
3. 记住常见验证失败原因与对应报错文案

## 小节索引

| 小节 | 笔记 |
|---|---|
| 6.1–6.2 | [验证机制与日志](./notes/6.1_验证机制与日志.md) |
| 6.3–6.4 | [典型验证失败与完成保证](./notes/6.3_典型验证失败与完成保证.md) |
| 6.5–6.7 | [坑点HFT关联与自测](./notes/6.5_坑点HFT关联与自测.md) |

## 交叉引用

- 前置：`../chapter-03-anatomy-of-ebpf-program/`（eBPF 虚拟机 10 寄存器、全局变量=map）、`../chapter-05-core-btf-libbpf/`（-g 编译出 BTF、libbpf_set_print）
- 后续：`../chapter-07-program-attachment-types/`（helper 可用性由程序类型决定、kfunc）、`../chapter-08-networking/`（XDP 的 data/data_end 边界检查实战）
