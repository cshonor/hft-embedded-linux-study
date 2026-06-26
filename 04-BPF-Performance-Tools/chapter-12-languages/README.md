# Ch 12 语言 · Languages

> **BPF Performance Tools** · Brendan Gregg · **跳过 ⚪**

> 本章定位：**按语言类型决定怎么挂 BPF** — 追踪前先问：底层是 **编译型 / JIT / 解释型**？符号从哪来？栈怎么 walk？参数怎么读？  
> **HFT：** 热路径 **C/C++/Rust** 必读 **帧指针 + 符号** 两节；共置 **Java/Go 辅助服务** 按需；**Go `uretprobe` 禁用**。与 [Ch 2 § 栈遍历](../chapter-02-technology-background/)、[Ch 13 应用案例](../chapter-13-applications/) 衔接。  
> **上一章：** [chapter-11-安全.md](../chapter-11-security/) · **下一章：** [chapter-13-应用程序.md](../chapter-13-applications/)

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1 本章核心问题 | [notes/section-1-本章核心问题.md](./notes/section-1-本章核心问题.md) |
| 2 编译型语言 (C, C++, Rust, Go…) | [notes/section-2-编译型语言.md](./notes/section-2-编译型语言.md) |
| 3 JIT 编译型 (Java, Node.js…) | [notes/section-3-JIT编译型.md](./notes/section-3-JIT编译型.md) |
| 4 解释型语言 (Bash, Python, Ruby…) | [notes/section-4-解释型语言.md](./notes/section-4-解释型语言.md) |
| 5 语言类型决策树 | [notes/section-5-语言类型决策树.md](./notes/section-5-语言类型决策树.md) |
| 6 与全书工具的关系 | [notes/section-6-与全书工具的关系.md](./notes/section-6-与全书工具的关系.md) |

---

## 大白话

> 按语言类型决定怎么挂 BPF

下面按原书小节展开；细节见 **小节笔记** 表。

---

## 本章 Checklist

- [ ] **策略核心（C++/Rust）**— 构建链：**frame pointer + debuginfo**；否则 Ch 6 `profile` 半盲。
- [ ] **USDT > uprobe**— 高频路径预埋静态探针；与 [Ch 2](../chapter-02-technology-background/) 原则一致。
- [ ] **Go：禁止 uretprobe**— 共置 Go 服务用 **pprof/trace**，BPF 只看内核边界。
- [ ] **Java/Node 辅助服务**— `PreserveFramePointer` + perf-map；**勿开** ExtendedDTrace 级 method 探针。
- [ ] **Python/Bash**— 运维脚本层；BPF 追 bash 内部仅 **取证/调试**。
- [ ] **C++ `this`**— 读 uprobe 参数时 **arg0 偏移**。

---

## 相关章节

- 上一章：[chapter-11-安全.md](../chapter-11-security/)
- 下一章：[chapter-13-应用程序.md](../chapter-13-applications/)
- 栈与 USDT：[chapter-02-技术背景.md](../chapter-02-technology-background/)
- CPU profile：[chapter-06-CPU.md](../chapter-06-cpus/)
- CSAPP 编译：[chapter-05-optimizing-performance](../01-CSAPP-3rd/chapter-05-optimizing-performance/)
- Rust 工程：[16-Rust-Quant-Trading-Guide](../16-Rust-Quant-Trading-Guide/)
