# Learning eBPF · 第 5 章：CO-RE——一次编译，处处运行

> **原书：** Chapter 5: CO-RE, BTF and Libbpf  
> **HFT：** 🔴 · **底本：** LEARNING-EBPF-BILINGUAL.pdf（GPT 双语逐段对照；PDF 存仓库外 `~/Desktop/hft-local-books/`，不入库）

> 全书最长的一章，讲清楚 eBPF 程序如何跨内核版本可移植：BTF 记录类型布局 → Clang 生成 CO-RE 重定位 → libbpf 加载时按目标内核改写指令。也是作者明确表态"BCC 不适合生产分发"的一章。

## 本章目标

1. 理解跨内核可移植性问题为什么存在，BCC 运行时编译方案的五大痛点
2. 掌握 CO-RE 五要素：BTF、内核头文件、编译器支持、重定位库、（可选）BPF skeleton
3. 能读懂 BTF 类型转储、`vmlinux.h` 生成、`bpf_core_relo` 重定位日志
4. 会写 libbpf 风格的内核侧 `.bpf.c` 与用户侧骨架代码

## 小节索引

| 小节 | 笔记 |
|---|---|
| 5.1–5.2 | [BCC之弊与CO-RE要素](./notes/5.1_BCC之弊与CO-RE要素.md) |
| 5.3–5.4 | [BTF与vmlinux](./notes/5.3_BTF与vmlinux.md) |
| 5.5–5.6 | [内核侧与用户侧实现](./notes/5.5_内核侧与用户侧实现.md) |
| 5.7–5.9 | [坑点HFT关联与自测](./notes/5.7_坑点HFT关联与自测.md) |

## 交叉引用

- 前置：`../chapter-04-bpf-syscall/`（BPF_BTF_LOAD、btf_fd 字段）、`../chapter-03-anatomy-of-ebpf-program/`（clang -target bpf、bpftool load）
- 后续：`../chapter-06-verifier/`（为什么不能直接 `p->y`）、`../chapter-07-program-attachment-types/`（SEC 名与程序类型全集）、`../chapter-08-networking/`（vmlinux.h 缺协议常量的实际案例）、`../chapter-10-programming/`（cilium/ebpf、libbpfgo、Aya）
