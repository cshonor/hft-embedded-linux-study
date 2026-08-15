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

| 节 | 笔记 |
|----|------|
| 1. eBPF 虚拟机 | [notes/section-1-eBPF虚拟机.md](./notes/section-1-eBPF虚拟机.md) |
| 2. XDP 版 Hello World（纯 C） | [notes/section-2-XDP版HelloWorld（纯C）.md](./notes/section-2-XDP版HelloWorld（纯C）.md) |
| 3. 编译与检查 | [notes/section-3-编译与检查.md](./notes/section-3-编译与检查.md) |
| 4. 加载、检查、附加、卸载（bpftool） | [notes/section-4-加载、检查、附加、卸载（bpftool）.md](./notes/section-4-加载、检查、附加、卸载（bpftool）.md) |
| 5. 全局变量 = map | [notes/section-5-全局变量=map.md](./notes/section-5-全局变量=map.md) |
| 6. BPF to BPF 调用（libbpf 才有） | [notes/section-6-BPFtoBPF调用（libbpf才有）.md](./notes/section-6-BPFtoBPF调用（libbpf才有）.md) |
| 坑点清单 | [notes/section-7-坑点清单.md](./notes/section-7-坑点清单.md) |
| HFT 关联 | [notes/section-8-HFT关联.md](./notes/section-8-HFT关联.md) |
| 自测题 | [notes/section-9-自测题.md](./notes/section-9-自测题.md) |

## 交叉引用

- 加载/附加在 syscall 层的全过程 → `../chapter-04-bpf-syscall/`
- BTF 与 CO-RE → `../chapter-05-core-btf-libbpf/`
- XDP 深入 → `../chapter-08-networking/`
- 指令集全景 → 本书附录 E；BPF 之巅附录同主题笔记 `../appendix-E-BPF指令.md`
