# 06.6 · Learning eBPF — Liz Rice

> **定位：** eBPF 原理入门书 — map、验证器、程序/附加类型、libbpf + CO-RE 现代开发栈
> **前置：** Linux 基础（[03-TLPI](../03-linux-userspace-api/) · [05-LKD](../05-linux-kernel/)）即可，**不需要**先读性能书
> **阅读顺序：** `06.6` → [06.6.5-SysPerf](../06.6.5-systems-performance/) → [06.7-BPT](../06.7-bpf-observability/)
> （本書与 06.6.5 可平行互换；06.7 BPT 是前两者的交汇，必须压轴）
> **后续：** [14-HFT](../14-hft-engineering/) / [18-Rust-quant](../18-rust-quant/)

---

## 目录结构

```
06.6-learning-ebpf/
├── README.md                  ← 本文件
├── LEARNING-EBPF-NOTES.md     ← 导读索引（含真机实验回灌表）
└── chapter-01~11-*/           每章一文件夹
    ├── README.md              章导读（目标/小节索引/交叉引用）
    ├── notes/                 按节拆分的笔记
    └── code/                  示例程序（BCC Python / libbpf .bpf.c / bpftrace .bt）
```

## 章节速览

| 章 | 主题 | 笔记 |
|----|------|------|
| 01 | eBPF 是什么 | [chapter-01-what-is-ebpf/](./chapter-01-what-is-ebpf/) |
| 02 | Hello World（BCC） | [chapter-02-hello-world/](./chapter-02-hello-world/) |
| 03 | 程序解剖（libbpf 风格） | [chapter-03-anatomy-of-ebpf-program/](./chapter-03-anatomy-of-ebpf-program/) |
| 04 | bpf(2) 系统调用 | [chapter-04-bpf-syscall/](./chapter-04-bpf-syscall/) |
| 05 | **CO-RE / BTF / libbpf**（核心章） | [chapter-05-core-btf-libbpf/](./chapter-05-core-btf-libbpf/) |
| 06 | 验证器 | [chapter-06-verifier/](./chapter-06-verifier/) |
| 07 | 程序附加类型 | [chapter-07-program-attachment-types/](./chapter-07-program-attachment-types/) |
| 08 | 网络（XDP/TC/uprobe-SSL） | [chapter-08-networking/](./chapter-08-networking/) |
| 09 | 安全（BPF LSM） | [chapter-09-security/](./chapter-09-security/) |
| 10 | bpftrace 编程 | [chapter-10-programming/](./chapter-10-programming/) |
| 11 | 未来展望 | [chapter-11-future/](./chapter-11-future/) |

## 相关

- 工具谱系（姊妹书） → [06.7-bpf-observability](../06.7-bpf-observability/)
- 双书对比 → [EBPF-BOOKS-COMPARISON.md](../06.7-bpf-observability/EBPF-BOOKS-COMPARISON.md)
- 性能方法论 → [06.6.5-systems-performance](../06.6.5-systems-performance/)
- 配套真机实验仓：`cshonor/ebpf-gate`（树莓派 5 / aarch64，lab 结论已回灌各章笔记）
