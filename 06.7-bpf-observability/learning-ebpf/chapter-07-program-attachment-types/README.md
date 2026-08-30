# Learning eBPF · 第 7 章：程序类型与附加类型

> **原书：** Chapter 7: eBPF Program and Attachment Types  
> **HFT：** 🔴 · **底本：** LEARNING-EBPF-BILINGUAL.pdf（GPT 双语逐段对照；PDF 存仓库外 `~/Desktop/hft-local-books/`，不入库）

> 当前内核约 30 种程序类型、40+ 种附加类型。本章回答三个问题：程序类型决定什么？追踪类有哪些选型？网络类有哪些挂点？

## 本章目标

1. 理解"程序类型 → 可附加事件 → 上下文结构 → 可用 helper/kfunc → 返回码语义"这条决定链
2. 掌握追踪类选型：kprobe / kretprobe / fentry / fexit / tp / raw_tp / tp_btf 的取舍
3. 了解网络类挂点全景：socket → TC → XDP → flow dissector → LWT → cgroup

## 小节索引

| 小节 | 笔记 |
|---|---|
| 7.1–7.3 | [程序类型全景](./notes/7.1_程序类型全景.md) |
| 7.4–7.6 | [坑点HFT关联与自测](./notes/7.4_坑点HFT关联与自测.md) |

## 交叉引用

- 前置：`../chapter-05-core-btf-libbpf/`（SEC 名、BPF_KPROBE_SYSCALL 宏、vmlinux.h）、`../chapter-06-verifier/`（helper 可用性检查、bpf_context access）
- 后续：`../chapter-08-networking/`（XDP/TC/socket filter 实战、uprobe 钩 SSL 明文）、`../chapter-09-security/`（LSM、syscall 探针可被绕过的原因）
