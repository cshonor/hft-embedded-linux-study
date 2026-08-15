# Learning eBPF · 第 4 章：bpf() 系统调用

> **原书：** Chapter 4: The bpf() System Call  
> **HFT：** 🔴 · **底本：** LEARNING-EBPF-BILINGUAL.pdf（GPT 双语逐段对照；PDF 存仓库外 `~/Desktop/hft-local-books/`，不入库）

> 用 strace 逐条解剖用户态到底发了哪些 syscall——这是全书最"内核视角"的一章，也是理解所有 eBPF 库（BCC/libbpf）本质的上限。

## 本章目标

1. 掌握 `bpf()` 系统调用签名与常用命令
2. 看懂 strace 下"加载程序 + 建 map + 挂事件 + 收数据"的完整 syscall 序列
3. 理解 BPF 对象的生命周期：fd → 引用计数 → pin → BPF link

## 小节索引

| 节 | 笔记 |
|----|------|
| 1. bpf() 总览 | [notes/section-1-bpf总览.md](./notes/section-1-bpf总览.md) |
| 2. strace 实例全景（hello-buffer-config.py） | [notes/section-2-strace实例全景（hello-buffer-config.py）.md](./notes/section-2-strace实例全景（hello-buffer-config.py）.md) |
| 3. BPF 对象生命周期：引用计数 | [notes/section-3-BPF对象生命周期：引用计数.md](./notes/section-3-BPF对象生命周期：引用计数.md) |
| 4. 挂 kprobe：bpf() 之外的三件套 | [notes/section-4-挂kprobe：bpf之外的三件套.md](./notes/section-4-挂kprobe：bpf之外的三件套.md) |
| 5. perf buffer 初始化：为什么是每核一个 | [notes/section-5-perfbuffer初始化：为什么是每核一个.md](./notes/section-5-perfbuffer初始化：为什么是每核一个.md) |
| 6. Ring buffer：单缓冲 + epoll | [notes/section-6-Ringbuffer：单缓冲+epoll.md](./notes/section-6-Ringbuffer：单缓冲+epoll.md) |
| 7. 遍历 map：bpftool map dump 的 syscall 序列 | [notes/section-7-遍历map：bpftoolmapdump的syscall序列.md](./notes/section-7-遍历map：bpftoolmapdump的syscall序列.md) |
| 坑点清单 | [notes/section-8-坑点清单.md](./notes/section-8-坑点清单.md) |
| HFT 关联 | [notes/section-9-HFT关联.md](./notes/section-9-HFT关联.md) |
| 自测题 | [notes/section-10-自测题.md](./notes/section-10-自测题.md) |

## 交叉引用

- BTF 数据的内部结构 → `../chapter-05-core-btf-libbpf/`
- BPF_PROG_LOAD 触发的验证流程 → `../chapter-06-verifier/`
- 各程序类型的附加方式 → `../chapter-07-program-attachment-types/`
- perf_event_open 通用机制 → 03-linux-userspace-api 模块 perf 相关笔记
