# Learning eBPF · 第 4 章：bpf() 系统调用

> **原书：** Chapter 4: The bpf() System Call  
> **HFT：** 🔴 · **底本：** LEARNING-EBPF-BILINGUAL.pdf（GPT 双语逐段对照；PDF 存仓库外 `~/Desktop/hft-local-books/`，不入库）

> 用 strace 逐条解剖用户态到底发了哪些 syscall——这是全书最"内核视角"的一章，也是理解所有 eBPF 库（BCC/libbpf）本质的上限。

## 本章目标

1. 掌握 `bpf()` 系统调用签名与常用命令
2. 看懂 strace 下"加载程序 + 建 map + 挂事件 + 收数据"的完整 syscall 序列
3. 理解 BPF 对象的生命周期：fd → 引用计数 → pin → BPF link

## 小节索引

| 原书小节 | 笔记 |
|---|---|
| §4.1–4.2 | [4.1 bpf总览与strace实例](./notes/4.1_bpf总览与strace实例.md) |
| §4.3–4.4 | [4.2 对象生命周期与kprobe挂载](./notes/4.2_对象生命周期与kprobe挂载.md) |
| §4.5–4.6 | [4.3 perf与RingBuffer](./notes/4.3_perf与RingBuffer.md) |
| §4.7 | [4.4 遍历map的syscall序列](./notes/4.4_遍历map的syscall序列.md) |
| §4.8–4.10 | [4.5 坑点HFT关联与自测](./notes/4.5_坑点HFT关联与自测.md) |

## 交叉引用

- BTF 数据的内部结构 → `../chapter-05-core-btf-libbpf/`
- BPF_PROG_LOAD 触发的验证流程 → `../chapter-06-verifier/`
- 各程序类型的附加方式 → `../chapter-07-program-attachment-types/`
- perf_event_open 通用机制 → 03-linux-userspace-api 模块 perf 相关笔记
