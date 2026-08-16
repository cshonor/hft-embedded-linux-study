# Linux Kernel Debugging — Kaiwan N. Billimoria

> **定位：** 内核**正确性调试** — 内核代码出了 BUG 怎么定位根因、怎么修
> **作者：** Kaiwan N. Billimoria · **出版：** Packt, 2022 · **页数：** 638 · **内核版本：** 5.x
> **硬件适配：** ✅ 明确支持 ARM / 树莓派（书中示例同时覆盖 x86 和 ARM）
>
> **前置：**
> - [05-linux-kernel](../05-linux-kernel/)（LKD，内核概念框架）
> - [18-linux-kernel-deep](../18-linux-kernel-deep/)（ULK3，内核深度原理）
> - [05.5-modern-kernel](../05.5-modern-kernel/)（现代 5.x/6.x 内核资料，修正旧书过时内容）
> - C 语言 + 内核模块开发基础
>
> **后续：**
> - [06-linux-mm](../06-linux-mm/)（内存管理子系统，调试 MM 问题时回来查）
> - [06.6-systems-performance](../06.6-systems-performance/)（性能分析，从"正确性"转向"性能"）
> - [06.7-bpf-observability](../06.7-bpf-observability/)（eBPF 动态追踪，从"调试"转向"可观测"）

---

## ⚠️ 与 06.6-systems-performance / 06.7-bpf-observability 的边界

| 维度 | 本书 (08.6) | 性能之巅 (19) | BPF之巅 (20) |
|------|-------------|---------------|--------------|
| **核心问题** | 内核为什么**坏了** | 系统为什么**慢了** | 内核**正在做什么** |
| **问题类型** | crash / Oops / 内存泄漏 / 数据竞争 / 死锁 | 延迟 / 吞吐量 / CPU 与 IO 瓶颈 | 事件级追踪 / 热点定位 |
| **视角** | 内核开发者（写代码 → 出 bug → 定位修复） | 性能工程师 / SRE（运行中系统 → 找瓶颈） | 平台工程师 / SRE（生产环境 → 实时观测） |
| **工具层级** | 内核内建调试框架（KASAN / KCSAN / KGDB / Kprobes） | 系统级观测工具（perf / top / iostat / Ftrace） | eBPF 动态追踪（bpftrace / BCC / libbpf） |
| **内核侵入性** | 高 — 需重编译内核启用调试选项 | 低 — 使用现有接口 | 极低 — 运行时注入，无需重编译 |
| **使用时机** | 开发阶段（写模块 / 驱动时） | 性能调优阶段（系统已运行但慢） | 生产环境（持续观测） |
| **Ftrace 覆盖** | Ch9：Ftrace 追踪内核流程辅助调试 | Ch14：Ftrace 作为性能分析工具 | Ch2：Ftrace 作为 eBPF 前身介绍 |
| **Kprobes 覆盖** | Ch4：Kprobes 作为调试探针深入讲解 | 提及但不深入 | Ch2：Kprobes 作为 eBPF 底层机制 |
| **内存覆盖** | Ch5-6：KASAN / UBSAN / SLUB debug / kmemleak | Ch7：内存性能（带宽 / 页面错误） | Ch7：内存事件（缺页 / 分配 / 回收） |
| **并发覆盖** | Ch8：LOCKDEP / KCSAN 数据竞争检测 | 不涉及 | Ch6：CPU 调度器追踪（间接相关） |
| **HFT 关联** | 自定义内核模块调试（定制网卡驱动 / 内核旁路） | 系统级延迟定位 | 生产环境实时延迟观测 |

### 三者关系一句话

> **本书**教你"内核坏了怎么修"（correctness）；
> **性能之巅**教你"系统慢了怎么找"（performance）；
> **BPF之巅**教你"内核在干什么怎么看"（observability）。
>
> 完整链路：先保证正确性（08.6）→ 再优化性能（19）→ 最后持续观测（20）。

### Ftrace / Kprobes 的重叠说明

三者都涉及 Ftrace 和 Kprobes，但**目的不同**，不会重复学习：

| 工具 | 本书 (08.6) 视角 | 性能之巅 (19) 视角 | BPF之巅 (20) 视角 |
|------|------------------|-------------------|------------------|
| Ftrace | 追踪内核函数调用链，定位 bug 触发路径 | 测量函数延迟，找性能瓶颈 | 作为 eBPF 的前身技术介绍 |
| Kprobes | 动态插入探针，捕获特定函数的参数和返回值 | 不深入 | 作为 eBPF 底层 tracing 机制介绍 |

**建议阅读顺序：** 先读本书 Ch4 (Kprobes) 和 Ch9 (Ftrace) 建立内核级 tracing 基础，再读 19 Ch14 (Ftrace 性能视角) 和 20 Ch2 (eBPF 技术背景) 时会有"原来如此"的贯通感。

---

## 阅读策略（HFT / 树莓派 5 / AArch64 / Linux 6.1）

### 精读章节（8 章）

| 章 | 标题 | 精读理由 |
|----|------|----------|
| 3 | printk and Friends | 内核调试的基石，动态调试框架 (dynamic debug) 在 6.x 仍是主力 |
| 4 | Kprobes | 动态探针，HFT 延迟溯源的核心工具之一；也是 eBPF 的底层机制 |
| 5 | Memory Issues Part 1 | KASAN / UBSAN / SLUB debug — 调试自写内核模块的内存错误 |
| 7 | Oops! Interpreting the Bug | 看 Oops 日志定位崩溃位置，内核开发必备技能 |
| 8 | Lock Debugging | LOCKDEP / KCSAN — 并发 bug 是内核中最难调的，HFT 自定义模块必用 |
| 9 | Tracing the Kernel Flow | Ftrace / trace-cmd / KernelShark — 延迟分析的关键工具链 |
| 10 | Kernel Panic, Lockups, and Hangs | soft/hard lockup 检测 — HFT 系统挂死时的第一诊断手段 |
| 11 | Using KGDB | 源码级单步调试 — 写内核模块时的高效调试方式 |

### 跳读章节（4 章）

| 章 | 标题 | 跳读理由 |
|----|------|----------|
| 1 | General Introduction | 调试历史案例（Patriot 导弹 / Ariane 5 / Mars Pathfinder），了解即可 |
| 2 | Approaches to Kernel Debugging | 方法论概览，快速翻阅 |
| 6 | Memory Issues Part 2 | KFENCE 等较新特性，按需查阅 |
| 12 | A Few More Approaches | GCOV/KCOV 代码覆盖率、syzkaller 模糊测试，做内核测试时再回来看 |

### 树莓派 5 / AArch64 适配说明

- 书中示例同时覆盖 x86 和 ARM，树莓派 5 (Cortex-A76, AArch64) 可直接运行大部分实验
- KGDB 需要串口调试线连接树莓派 5 的 UART（GPIO 14/15），书中 Ch11 有详细配置步骤
- KASAN 在 ARM64 上有完整支持，但需要 `CONFIG_KASAN=y` 重新编译内核
- Ftrace 在 ARM64 上功能完整，trace-cmd 和 KernelShark 可通过 apt 安装

---

## 目录结构

```
chapter-XX-english-slug/
├── README.md      ← 章导读（中文标题、精读/跳读标注、Checklist）
└── notes/         ← 按原书小节拆分的笔记
```

与 [05-linux-kernel](../05-linux-kernel/) · [18-linux-kernel-deep](../18-linux-kernel-deep/) · [06.6-systems-performance](../06.6-systems-performance/) · [06.7-bpf-observability](../06.7-bpf-observability/) 同一套约定。

---

## 全书章节（3 Part · 12 章）

### Part 1: Introduction & Approaches

| 章 | 标题 | 读/跳 | 目录 |
|----|------|-------|------|
| 1 | A General Introduction to Debugging Software | 跳读 | [chapter-01-introduction](./chapter-01-introduction/) |
| 2 | Approaches to Kernel Debugging | 跳读 | [chapter-02-approaches](./chapter-02-approaches/) |

### Part 2: Instrumentation & Memory Debugging

| 章 | 标题 | 读/跳 | 目录 |
|----|------|-------|------|
| 3 | Debug via Instrumentation — printk and Friends | 🔴 精读 | [chapter-03-printk](./chapter-03-printk/) |
| 4 | Debug via Instrumentation — Kprobes | 🔴 精读 | [chapter-04-kprobes](./chapter-04-kprobes/) |
| 5 | Debugging Kernel Memory Issues — Part 1 | 🔴 精读 | [chapter-05-memory-debug-1](./chapter-05-memory-debug-1/) |
| 6 | Debugging Kernel Memory Issues — Part 2 | 跳读 | [chapter-06-memory-debug-2](./chapter-06-memory-debug-2/) |

### Part 3: Diagnostics & Advanced Tools

| 章 | 标题 | 读/跳 | 目录 |
|----|------|-------|------|
| 7 | Oops! Interpreting the Kernel Bug Diagnostic | 🔴 精读 | [chapter-07-oops](./chapter-07-oops/) |
| 8 | Lock Debugging | 🔴 精读 | [chapter-08-lock-debug](./chapter-08-lock-debug/) |
| 9 | Tracing the Kernel Flow | 🔴 精读 | [chapter-09-ftrace](./chapter-09-ftrace/) |
| 10 | Kernel Panic, Lockups, and Hangs | 🔴 精读 | [chapter-10-panic-lockup](./chapter-10-panic-lockup/) |
| 11 | Using Kernel GDB (KGDB) | 🔴 精读 | [chapter-11-kgdb](./chapter-11-kgdb/) |
| 12 | A Few More Kernel Debugging Approaches | 跳读 | [chapter-12-misc](./chapter-12-misc/) |

---

## HFT 关联

| 场景 | 相关章节 | 说明 |
|------|----------|------|
| 自定义内核模块崩溃 | Ch7 (Oops) + Ch5 (KASAN) | 写内核旁路网络模块时，Oops 日志是第一现场 |
| 内核死锁导致交易系统挂死 | Ch8 (Lock Debug) + Ch10 (Lockup) | LOCKDEP 在开发期发现锁序问题，watchdog 在生产期检测挂死 |
| 延迟毛刺溯源 | Ch9 (Ftrace) + Ch4 (Kprobes) | Ftrace 追踪调度器延迟，Kprobes 捕获特定函数耗时 |
| 内核模块源码级调试 | Ch11 (KGDB) | 串口连接树莓派 5，单步调试自写内核模块 |
| 内存泄漏导致 OOM | Ch5-6 (Memory Debug) | kmemleak 检测内核模块的内存泄漏 |
