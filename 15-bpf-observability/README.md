# 15 · eBPF 观测 — 双书模块

**文件夹 15** · 两本 O'Reilly eBPF 书 · [返回总清单](../READING-LIST.md)

> **前置：** [14-Systems-Performance](../14-systems-performance/)  
> **建议时机：** 已有 Linux 内核/网络/DPDK 或 HFT 压测靶子后再开 — 用 eBPF 验证真实系统  
> **后续：** [16-HFT](../16-hft-engineering/) / [20-Rust](../20-rust-quant/)

---

## 目录结构（一书一目录）

```
15-bpf-observability/
├── learning-ebpf/            ← 第一本（原理入门）
│   ├── LEARNING-EBPF-NOTES.md   导读索引
│   └── chapter-01~11-*/         每章一文件夹
│       ├── README.md            章导读（目标/小节索引/交叉引用）
│       └── notes/               按节拆分的笔记
├── bpf-performance-tools/    ← 第二本（工具谱系）
│   ├── OUTLINE.md               全书目录 + HFT 读/跳标注
│   ├── BPF-PERFORMANCE-TOOLS-EVAL.md
│   ├── chapter-01~18/           章导读 + 分节笔记
│   ├── appendix-A~E             bpftrace/BCC/指令附录
│   └── note-XDP与tc-BPF.md      HFT 延伸
├── EBPF-BOOKS-COMPARISON.md  ← 两书对比与协同读法
├── ref-*.md                  ← 模块级参考（bpftrace 脚本/排查决策树/评审清单）
└── README.md
```

两本书同一套目录约定（与 [02-CSAPP](../02-computer-systems/) · [14-Systems-Performance](../14-systems-performance/) 一致）：`chapter-XX-english-slug/README.md`（章导读）+ `notes/`（按节拆分的笔记）。

---

## 📖 两本书与阅读顺序

| 顺序 | 书 | 笔记入口 | 定位 | 篇幅 |
|------|----|------|------|------|
| **1** | **Learning eBPF**（Liz Rice, 2023） | [LEARNING-EBPF-NOTES.md](./learning-ebpf/LEARNING-EBPF-NOTES.md) | **原理 + 写程序**：eBPF 是什么、map、验证器、程序/附加类型、libbpf + CO-RE 现代开发栈、网络/安全应用 | 11 章，快速建框架 |
| **2** | **BPF Performance Tools**（Brendan Gregg, 2019） | [OUTLINE.md](./bpf-performance-tools/OUTLINE.md) → [全书评析](./bpf-performance-tools/BPF-PERFORMANCE-TOOLS-EVAL.md) | **工具谱系 + 方法论**：BCC/bpftrace 按 CPU/内存/IO/网络资源域的观测工具百科与性能分析方法 | 18 章 + 附录 A–E，工具书 |

### 为什么是这个顺序

1. **认知规律：先懂原理，再用工具。** BPT 里的 bpftrace/BCC 工具，底层就是 Learning eBPF 第 7 章的程序/附加类型、第 6 章的验证器、第 2 章的 map。先读原理书，用工具报"invalid mem access"时能直接读懂验证器日志，而不是当黑盒。

2. **先薄后厚，先框架后细节。** Learning eBPF 11 章可较快过完，建立"事件 → 程序类型 → 上下文 → helper → map → 用户态"的完整心智模型；BPT 800+ 页是**按需查阅**的参考书——没有框架直接啃，容易迷失在工具列表里，有了框架则 Ch1–5 技术背景部分可以速通，直接进入 Ch6–16 工具使用。

3. **新旧顺序：先学现代标准。** Learning eBPF（2023）代表 libbpf + CO-RE 现代栈；BPT（2019）停在 BCC 运行时编译范式。先掌握现代标准，再读 BPT 时能分清哪些是历史写法、哪些工具思路至今有效。

4. **能力递进闭环：先会造，再会用。** Learning eBPF 教你**写**定制观测程序（HFT 需要非标准挂点，如行情网卡入口的 XDP、解析库的 uprobe）；BPT 教你**用**成熟工具快速覆盖标准场景（调度/缺页/TCP 重传）。前者补 HFT 定制能力，后者补系统性能分析方法论（USE 方法、下钻策略）。

> 详细对比（含第三本 Linux Observability with BPF）见 [EBPF-BOOKS-COMPARISON.md](./EBPF-BOOKS-COMPARISON.md)。

---

## 第二本 · BPF Performance Tools 内部导航

各章导读见 [bpf-performance-tools/OUTLINE.md](./bpf-performance-tools/OUTLINE.md)（含 🔴🟡⚪ HFT 读/跳标注）。

**HFT 精读捷径：**

```
Ch 1–2 → Ch 4–5 → Ch 6 → Ch 10 (+ note-XDP) → 附录 A/B
```

## 模块级参考文档

| 文档 | 用途 |
|------|------|
| [bpftrace 样例脚本集](./ref-bpftrace-scripts.md) | 8 个场景脚本：调度/IO/TCP/syscall/锁/缺页/slab/软中断 |
| [故障排查决策树](./ref-troubleshooting-decision-tree.md) | 7 大症状入口 → 19 看现象 → 20 钻根因 |
| [Rubric 评审校验清单](./ref-rubric-checklist.md) | 一次排障/优化做完后逐项打勾（6 大类 30+ 检查项） |

## 交叉阅读

- **上一本（必读前置）** → [14-systems-performance](../14-systems-performance/) — 读完立刻读本目录
- 后续内核/内存/网络 → [05-linux-kernel](../05-linux-kernel/) · [06-linux-mm](../06-linux-mm/) · [12-kernel-networking](../12-kernel-networking/)（读时可回头用 eBPF 验证）
- DPDK 对照 → [13-dpdk](../13-dpdk/)（XDP early drop vs 用户态旁路）
- Rust eBPF → [20-rust-quant](../20-rust-quant/)（Aya/bpf2go）
- 跨模块 → [README.md](../README.md)
