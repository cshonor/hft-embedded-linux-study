# Learning eBPF · 第 8 章：eBPF 与网络

> **原书：** Chapter 8: eBPF for Networking  
> **HFT：** 🔴 · **底本：** LEARNING-EBPF-BILINGUAL.pdf（GPT 双语逐段对照；PDF 存仓库外 `~/Desktop/hft-local-books/`，不入库）

> 网络是 eBPF 最出成绩的领域：XDP 在网卡驱动层丢包、TC 在流量控制层改包、uprobe 钩 SSL 看明文、Cilium 用 eBPF 重写整个 K8s 数据面。本章回答：eBPF 能挂在网络路径的哪些位置？每个位置能做什么、怎么做？

## 本章目标

1. 掌握 XDP：五种返回码、`xdp_md` 上下文、包解析边界检查范式、负载均衡实战、硬件 offload
2. 掌握 TC 层：`sk_buff` 上下文、TC_ACT 返回码、与 XDP 的分工、多程序串联
3. 学会用 uprobe 钩 SSL_read/SSL_write 观测加密流量的明文（entry/retprobe 配对模式）
4. 理解 K8s 场景：iptables 的 O(n) 困境 vs eBPF hash map O(1)、Cilium 多程序协作、NetworkPolicy、透明加密与身份认证

## 小节索引

| 原书小节 | 笔记 |
|---|---|
| §8.1–8.3 | [8.1 网络路径XDP与TC](./notes/8.1_网络路径XDP与TC.md) |
| §8.4–8.5 | [8.2 uprobe与Kubernetes](./notes/8.2_uprobe与Kubernetes.md) |
| §8.6–8.8 | [8.3 坑点HFT关联与自测](./notes/8.3_坑点HFT关联与自测.md) |

## 交叉引用

- 第 7 章 `../chapter-07-program-attachment-types/`：XDP/SCHED_CLS 程序类型与附加方式、uprobe 基础、一接口一 XDP 程序的限制
- 第 6 章 `../chapter-06-verifier/`：边界检查为什么是验证器硬性要求、`pkt_end prohibited` 报错
- 第 9 章 `../chapter-09-security/`：uprobe 观测与安全审计的关系、syscall 探针可被绕过的问题
- 第 10 章 `../chapter-10-programming/`：用性能工具实测 XDP/TC 的处理开销
