# 《BPF 之巅：Linux 系统可观测性技术》评测与路线定位

> **英文：** *BPF Performance Tools*  
> **作者：** Brendan Gregg（Netflix 首席性能工程师）  
> **出版：** 2019（Addison-Wesley / O'Reilly 系动物封面）  
> **内核：** 4.x 时代（写作时主流 4.15–5.0）  
> **模块：** [15-bpf-observability/](./) · **主书**  
> **本地电子书：** 《BPF之巅》中文版（孙宇聪/吕宏利/刘晓舟 译，扫描版，858 页）— 版权原因不入库，存 `~/Desktop/hft-local-books/`  
> **章节地图：** [OUTLINE.md](./OUTLINE.md)（18 章 + 附录 A–E，含 HFT 读/跳标注）  
> **姊妹对比：** [EBPF-BOOKS-COMPARISON.md](.././EBPF-BOOKS-COMPARISON.md)

---

## 一句话结论

**eBPF 工具谱系百科全书** — Brendan Gregg 把"用 BPF 看什么指标"讲到了极致。  
本模块主书，**按 [OUTLINE](./OUTLINE.md) 的 🔴 标注精读工具章节即可**；原理全貌补 [Learning eBPF](../learning-ebpf/LEARNING-EBPF-NOTES.md)。

---

## 定位

| | |
|--|--|
| 类型 | 性能分析工具谱系；**用工具** 而非写程序 |
| 强项 | 18 章 + 附录覆盖 CPU/内存/磁盘/网络/安全/容器全场景；BCC + bpftrace 双工具链；火焰图、off-CPU、USE 方法落地 |
| 弱项 | 2019 截止，**无 CO-RE / libbpf 现代开发栈**；BCC Python 脚本范式已过时；内核 5.x+ 新特性（bpf_iter、links、BTF CO-RE）未覆盖 |
| 适合谁 | 运维 / 性能工程师 / HFT 调优 — 要"现场用工具定位延迟"的人 |

---

## 优势

1. **工具视角最全** — 每类资源（CPU/内存/网络…）都给出"看哪个指标、用什么 bpftrace 单行、怎么解读火焰图"
2. **Brendan Gregg 方法论落地** — USE 方法、off-CPU 分析、延迟分解都在真实工具上演示
3. **bpftrace 单行命令 + 备忘单** — 附录 A/B 是现场速查金矿，已整理为 [appendix-A](./appendix-A-bpftrace单行命令.md) / [appendix-B](./appendix-B-bpftrace备忘单.md)
4. **HFT 热路径章节扎实** — Ch6 CPU（off-CPU、run queue、抖动）、Ch10 网络（套接字延迟、TCP 重传）、Ch14 内核调度唤醒
5. **XDP / tc-BPF** — 本模块补充 [note-XDP与tc-BPF.md](./note-XDP与tc-BPF.md)，与 DPDK 内核旁路对照

---

## 局限（2026 年视角）

1. **无 CO-RE / libbpf** — 现代 eBPF 开发已转向 libbpf + CO-RE（编译一次到处运行），本书 BCC Python 脚本范式在生产中逐渐淘汰
2. **内核 5.x+ 新特性缺失** — bpf_iter、struct ops、BTF CO-RE、links/freplace、bpf_timer 等都没讲
3. **不教你写 eBPF 程序** — 纯工具使用；想自己写程序看 [Learning eBPF](../learning-ebpf/LEARNING-EBPF-NOTES.md)
4. **部分工具已演进** — bpftrace 语法、BCC 工具名有变动，实际使用以最新版为准

> 这些局限正是引入 [Learning eBPF](../learning-ebpf/LEARNING-EBPF-NOTES.md) 作为现代补充的原因。

---

## HFT 价值与读法

| 读法 | 章节 | 为什么 |
|------|------|--------|
| 🔴 精读 | Ch1–2 背景、Ch4 BCC、Ch5 bpftrace | 工具链上手 + eBPF 原理最小集 |
| 🔴 精读 | Ch6 CPU、Ch10 网络、附录 A/B | HFT 抖动定位的核心：off-CPU、run queue、套接字延迟、bpftrace 速查 |
| 🟡 选读 | Ch3 性能分析、Ch7 内存、Ch13 应用、Ch14 内核 | 补充视角，按需 |
| ⚪ 跳过 | Ch8–9 文件系统/磁盘、Ch11 安全、Ch12 语言、Ch15–16 容器/虚拟化 | HFT 热路径无关 |

**HFT 产出：** 生产环境 eBPF 观测能力；与 [DPDK](../../13-dpdk/) 配合做「内核栈 vs 用户态旁路」对比；用 bpftrace 定位软中断、调度抖动、网卡队列堆积。

**精读捷径：**
```
Ch 1–2 → Ch 4–5 → Ch 6 → Ch 10 (+ XDP note) → 附录 A/B
```

---

## 与其他 eBPF 书的关系

| 书 | 视角 | 互补点 |
|----|------|--------|
| **BPF Performance Tools**（本书） | 用工具看指标 | 工具谱系 + 性能分析方法论 |
| [Learning eBPF](../learning-ebpf/LEARNING-EBPF-NOTES.md) | 写 eBPF 程序 | libbpf/CO-RE 现代栈、验证器、程序类型 |
| Linux Observability with BPF | 运维观测案例 | 更老，与本书重叠，不必入 |

**协同读法：** 本书练工具（怎么用）→ Learning eBPF 补原理与现代开发栈（怎么写）。

---

## 本模块已有笔记

| 部分 | 笔记 |
|------|------|
| 全书目录 + HFT 标注 | [OUTLINE.md](./OUTLINE.md) |
| 模块导读 | [README.md](.././README.md) |
| XDP / tc-BPF 补充 | [note-XDP与tc-BPF.md](./note-XDP与tc-BPF.md) |
| bpftrace 单行命令 | [appendix-A](./appendix-A-bpftrace单行命令.md) |
| bpftrace 备忘单 | [appendix-B](./appendix-B-bpftrace备忘单.md) |
| BCC 工具开发 | [appendix-C](./appendix-C-BCC工具开发.md) |
| C 语言 BPF | [appendix-D](./appendix-D-C语言BPF.md) |
| BPF 指令 | [appendix-E](./appendix-E-BPF指令.md) |
| 三书对比 | [EBPF-BOOKS-COMPARISON.md](.././EBPF-BOOKS-COMPARISON.md) |
