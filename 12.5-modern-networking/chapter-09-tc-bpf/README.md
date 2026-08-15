# Chapter 09: TC 与 BPF

> 来源：Bootlin（TC 概述）+ LWN（TC BPF）
> 对标：Rosen Ch8（TC 3.x → TC BPF 6.x）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [tc-bootlin](notes/01-tc-bootlin.md) | Bootlin：TC qdisc、class、filter、BPF 分类器 |
| 2 | [tc-bpf](notes/02-tc-bpf.md) | LWN：tc-bpf 程序、clsact qdisc、ingress/egress hook |

## HFT 关联

- **clsact qdisc**：无队列分类器，ingress/egress hook 不引入排队延迟
- **TC BPF egress**：在发包路径插入 BPF 程序，可修改/丢弃/重定向包
- **TC vs XDP**：TC 在协议栈之后（可操作 sk_buff），XDP 在协议栈之前（仅 xdp_buff）；HFT 优先用 XDP
- **fq qdisc**：fq qdisc 配合 pacing 可控制发包速率，避免 burst 导致交换机 buffer 溢出

## 交叉引用

- `12.5-modern-networking/chapter-03-tx-path-skbbuff/`：TC 在发包路径的位置
- `12.5-modern-networking/chapter-08-ebpf-cgroup-bpf/`：eBPF 通用框架
