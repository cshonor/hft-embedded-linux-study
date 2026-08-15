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

| 节 | 笔记 |
|----|------|
| 1. eBPF 在网络路径上的位置 | [notes/section-1-eBPF在网络路径上的位置.md](./notes/section-1-eBPF在网络路径上的位置.md) |
| 2. XDP（eXpress Data Path） | [notes/section-2-XDP（eXpressDataPath）.md](./notes/section-2-XDP（eXpressDataPath）.md) |
| 3. TC（Traffic Control）层 | [notes/section-3-TC（TrafficControl）层.md](./notes/section-3-TC（TrafficControl）层.md) |
| 4. uprobe 钩 SSL：看加密流量的明文 | [notes/section-4-uprobe钩SSL：看加密流量的明文.md](./notes/section-4-uprobe钩SSL：看加密流量的明文.md) |
| 5. Kubernetes 与 eBPF（Cilium 视角） | [notes/section-5-Kubernetes与eBPF（Cilium视角）.md](./notes/section-5-Kubernetes与eBPF（Cilium视角）.md) |
| 6. 坑点清单 | [notes/section-6-坑点清单.md](./notes/section-6-坑点清单.md) |
| 7. HFT 关联 | [notes/section-7-HFT关联.md](./notes/section-7-HFT关联.md) |
| 8. 自测题 | [notes/section-8-自测题.md](./notes/section-8-自测题.md) |

## 交叉引用

- 第 7 章 `../chapter-07-program-attachment-types/`：XDP/SCHED_CLS 程序类型与附加方式、uprobe 基础、一接口一 XDP 程序的限制
- 第 6 章 `../chapter-06-verifier/`：边界检查为什么是验证器硬性要求、`pkt_end prohibited` 报错
- 第 9 章 `../chapter-09-security/`：uprobe 观测与安全审计的关系、syscall 探针可被绕过的问题
- 第 10 章 `../chapter-10-programming/`：用性能工具实测 XDP/TC 的处理开销
