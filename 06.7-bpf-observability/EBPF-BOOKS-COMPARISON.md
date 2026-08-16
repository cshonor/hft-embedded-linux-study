# eBPF O'Reilly 三书对比与阅读顺序

> 本模块主书为 **BPF Performance Tools**（Brendan Gregg，详见 [OUTLINE](./bpf-performance-tools/OUTLINE.md)）。
> 本文对比另外两本 O'Reilly eBPF 动物书，并给出三条路线的协同读法。

---

## 三本书一览

| # | 书名 | 作者 | 年代/内核 | 定位 | 现代度 |
|---|------|------|-----------|------|--------|
| ① | **Linux Observability with BPF**（《Linux 内核观测技术 BPF》） | David Calavera、Lorenzo Fontana（Sysdig/Falco） | 较早，偏 4.x | 可观测性 / 追踪 / 运维实战 | ⚠️ 缺 CO-RE、libbpf、现代开发流程 |
| ② | **Learning eBPF**（《eBPF 学习手册》） | Liz Rice（eBPF 基金会核心） | 新，5.x–6.x | 从零理解 eBPF 全貌，原理→写程序 | ✅ 现代，入门首选 |
| ③ | **BPF Performance Tools**（本模块主书） | Brendan Gregg | 4.x 时代，工具大全 | 性能分析工具谱系（BCC/bpftrace） | 🟡 工具视角，CO-RE 较少但案例极丰富 |

---

## ① Linux Observability with BPF — 观测实战补充

- **重点**：bcc 工具、trace 跟踪、系统观测案例；偏运维、故障排查
- **长板**：实战案例多，Falco/Sysdig 一线工程师视角
- **短板**：缺少 CO-RE、libbpf、现代 eBPF 开发流程，很多新特性未覆盖
- **对你（HFT/嵌入式）的值**：当工具案例库翻阅，挑追踪/观测章节即可

## ② Learning eBPF — 现代入门首选 ✅

- **重点**：eBPF 底层概念、libbpf、CO-RE、网络、安全、可观测；从原理到写 eBPF 程序
- **定位**：从零理解 eBPF 整体全貌，适合内核 / 嵌入式 / HFT 方向
- **为什么优先**：覆盖 5.x–6.x 内核与现代开发栈（libbpf + CO-RE），不会让你学一套即将过时的 bcc Python 脚本范式

## ③ BPF Performance Tools — 本模块主书

- 18 章 + 附录 A–E 的工具谱系，HFT 读/跳标注见 [OUTLINE](./bpf-performance-tools/OUTLINE.md)
- 与 ② 互补：② 讲"怎么写 eBPF 程序"，③ 讲"已有的 BPF 工具怎么用、看什么指标"

---

## 三书协同读法（HFT 路径）

```
② Learning eBPF          建立整体概念（eBPF 是什么、怎么跑、libbpf/CO-RE 现代栈）
        ↓
③ BPF Performance Tools  工具谱系 + 性能分析视角（本模块主线，按 OUTLINE 标注读）
        ↓
① Linux Observability    观测/trace 实战案例补充（挑章节翻，不逐页）
```

- **先 ② 后 ③**：② 给原理全貌，③ 给工具全貌；先有原理框架再看工具才不迷失
- **① 当补充**：部分内容老旧，重点看 trace/观测案例，跳过 bcc Python 老范式细节
- **HFT 落脚点**：eBPF 在 HFT 用来做内核跟踪、延迟观测、网络抓包、定位抖动——② 的网络/可观测 + ③ 的 Ch6 CPU / Ch10 Networking / Ch14 Kernel 正好覆盖

---

## 极简阅读顺序

### ② Learning eBPF（优先）
- **必读**：eBPF 底层概念、libbpf、CO-RE、网络、可观测章节
- **可跳**：纯安全策略章节（除非做安全方向）

### ③ BPF Performance Tools（本模块，详见 [OUTLINE](./bpf-performance-tools/OUTLINE.md)）
- **🔴 精读**：Ch1–2 背景、Ch4 BCC、Ch5 bpftrace、Ch6 CPU、Ch10 Networking
- **🟡 选读**：Ch3 性能分析、Ch7 内存、Ch13 应用、Ch14 内核
- **⚪ 跳过**：Ch8 文件系统、Ch9 磁盘 I/O、Ch11 安全、Ch12 语言、Ch15 容器、Ch16 虚拟机（HFT 关联弱）

### ① Linux Observability with BPF（补充）
- **看**：trace 跟踪、系统观测案例
- **跳**：bcc Python 脚本逐行讲解、已过时的老内核特性
