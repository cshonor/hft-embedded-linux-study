# Learning eBPF · 第 10 章：eBPF 编程

> **原书：** Chapter 10: eBPF Programming  
> **HFT：** 🟡 · **底本：** LEARNING-EBPF-BILINGUAL.pdf（GPT 双语逐段对照；PDF 存仓库外 `~/Desktop/hft-local-books/`，不入库）

> 要自己写 eBPF，必须同时处理两半：内核侧 eBPF 程序 + 用户侧加载/附加/读数据的代码。本章是选型地图：bpftrace（一行搞定）→ BCC（Python 快速原型）→ libbpf / ebpf-go / libbpfgo / Aya（CO-RE 生产级）。

## 本章目标

1. 会用 bpftrace 一行脚本和 opensnoop 式多程序协作脚本
2. 理解内核侧语言约束：为什么只有 C 和 Rust（无运行时、单线程）
3. 掌握各用户态库的定位与取舍：BCC、libbpf、ebpf-go、libbpfgo、libbpf-rs、Redbpf、Aya
4. 了解 BPF_PROG_RUN 测试与 `bpf_stats_enabled` 运行时统计
5. 理解"多 eBPF 程序 + map 协调"的标准应用形态

## 小节索引

| 小节 | 笔记 |
|---|---|
| 10.1–10.3 | [语言与库全景](./notes/10.1_语言与库全景.md) |
| 10.4–10.6 | [测试统计与程序协作](./notes/10.4_测试统计与程序协作.md) |
| 10.7–10.9 | [坑点HFT关联与自测](./notes/10.7_坑点HFT关联与自测.md) |

## 交叉引用

- 第 2 章 `../chapter-02-hello-world/`：BCC 第一个例子
- 第 5 章 `../chapter-05-core-btf-libbpf/`：BCC 五痛点、libbpf 骨架、`bpftool gen skeleton`
- 第 3 章 `../chapter-03-anatomy-of-ebpf-program/`：字节码结构（本章练习建议用 llvm-objdump 对比）
- 第 4 章 `../chapter-04-bpf-syscall/`：`strace -e bpf` 观察库的加载行为
- 第 9 章 `../chapter-09-security/`：TOCTOU——syscall 入口工具不能当安全防线
