# Learning eBPF · 第 3 章：eBPF 程序解剖

> **原书：** Chapter 3: Anatomy of an eBPF Program  
> **HFT：** 🔴 · **底本：** LEARNING-EBPF-BILINGUAL.pdf（GPT 双语逐段对照；PDF 存仓库外 `~/Desktop/hft-local-books/`，不入库）

> 抛弃 BCC 的黑盒，用纯 C + libbpf + bpftool 走完源码 → 字节码 → 机器码 → 加载 → 挂载全流程。

## 本章目标

1. 理解 eBPF 虚拟机：10 个软件寄存器 + 8 字节定长指令
2. 亲手编译（clang -target bpf）、检查（llvm-objdump/bpftool）、加载、附加、卸载
3. 全局变量 = map 语义（.bss/.rodata）
4. BPF to BPF 函数调用在字节码层的形态

## 小节索引

| 原书小节 | 笔记 |
|---|---|
| §3.1–3.2 | [3.1 虚拟机与XDP版HelloWorld](./notes/3.1_虚拟机与XDP版HelloWorld.md) |
| §3.3–3.4 | [3.2 编译检查与加载附加](./notes/3.2_编译检查与加载附加.md) |
| §3.5–3.6 | [3.3 全局变量与BPFtoBPF调用](./notes/3.3_全局变量与BPFtoBPF调用.md) |
| §3.7–3.9 | [3.4 坑点HFT关联与自测](./notes/3.4_坑点HFT关联与自测.md) |

## 交叉引用

- 加载/附加在 syscall 层的全过程 → `../chapter-04-bpf-syscall/`
- BTF 与 CO-RE → `../chapter-05-core-btf-libbpf/`
- XDP 深入 → `../chapter-08-networking/`
- 指令集全景 → 本书附录 E；BPF 之巅附录同主题笔记 `../appendix-E-BPF指令.md`
