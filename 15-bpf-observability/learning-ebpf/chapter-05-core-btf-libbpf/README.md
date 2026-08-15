# Learning eBPF · 第 5 章：CO-RE——一次编译，处处运行

> **原书：** Chapter 5: CO-RE, BTF and Libbpf  
> **HFT：** 🔴 · **底本：** [LEARNING-EBPF-BILINGUAL.pdf](../LEARNING-EBPF-BILINGUAL.pdf)（GPT 双语逐段对照）

> 全书最长的一章，讲清楚 eBPF 程序如何跨内核版本可移植：BTF 记录类型布局 → Clang 生成 CO-RE 重定位 → libbpf 加载时按目标内核改写指令。也是作者明确表态"BCC 不适合生产分发"的一章。

## 本章目标

1. 理解跨内核可移植性问题为什么存在，BCC 运行时编译方案的五大痛点
2. 掌握 CO-RE 五要素：BTF、内核头文件、编译器支持、重定位库、（可选）BPF skeleton
3. 能读懂 BTF 类型转储、`vmlinux.h` 生成、`bpf_core_relo` 重定位日志
4. 会写 libbpf 风格的内核侧 `.bpf.c` 与用户侧骨架代码

## 小节索引

| 节 | 笔记 |
|----|------|
| 1. BCC 的老方案为什么不行 | [notes/section-1-BCC的老方案为什么不行.md](./notes/section-1-BCC的老方案为什么不行.md) |
| 2. CO-RE 五要素 | [notes/section-2-CO-RE五要素.md](./notes/section-2-CO-RE五要素.md) |
| 3. BTF 深入 | [notes/section-3-BTF深入.md](./notes/section-3-BTF深入.md) |
| 4. vmlinux.h：一个头文件替代全部内核头 | [notes/section-4-vmlinux.h：一个头文件替代全部内核头.md](./notes/section-4-vmlinux.h：一个头文件替代全部内核头.md) |
| 5. 内核侧代码（hello-buffer-config.bpf.c） | [notes/section-5-内核侧代码（hello-buffer-config.bpf.c）.md](./notes/section-5-内核侧代码（hello-buffer-config.bpf.c）.md) |
| 6. 用户侧：libbpf + BPF Skeleton | [notes/section-6-用户侧：libbpf+BPFSkeleton.md](./notes/section-6-用户侧：libbpf+BPFSkeleton.md) |
| 7. 坑点清单 | [notes/section-7-坑点清单.md](./notes/section-7-坑点清单.md) |
| 8. HFT 关联 | [notes/section-8-HFT关联.md](./notes/section-8-HFT关联.md) |
| 9. 自测题 | [notes/section-9-自测题.md](./notes/section-9-自测题.md) |

## 交叉引用

- 前置：`../chapter-04-bpf-syscall/`（BPF_BTF_LOAD、btf_fd 字段）、`../chapter-03-anatomy-of-ebpf-program/`（clang -target bpf、bpftool load）
- 后续：`../chapter-06-verifier/`（为什么不能直接 `p->y`）、`../chapter-07-program-attachment-types/`（SEC 名与程序类型全集）、`../chapter-08-networking/`（vmlinux.h 缺协议常量的实际案例）、`../chapter-10-programming/`（cilium/ebpf、libbpfgo、Aya）
