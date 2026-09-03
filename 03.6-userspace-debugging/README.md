# 用户态调试（Userspace Debugging）

> **定位：** 用户态**正确性调试** — 用户态代码崩了、错了、内存泄漏了、卡死了，怎么定位根因、怎么修
> **形态：** 工具链实战模块（非单一书目）— 以 gdb / strace / valgrind / sanitizer / perf 官方手册为主干，辅以经典资料
> **目标平台：** x86_64（本地，主战场）+ AArch64（树莓派 5，通过 gdbserver 远程调试）

> **前置：**
> - [03-linux-userspace-api](../03-linux-userspace-api/)（TLPI，被调试的对象——用户态 syscall 代码）
> - [03.5-unix-network-api](../03.5-unix-network-api/)（UNP，网络程序的调试对象）
> - [01-c-language](../01-c-language/)（C / 指针，内存 bug 的根源）

> **后续：**
> - [04-cpp](../04-cpp/)（C++，调试多态 / 模板 / STL 代码时回来查）
> - [06.6-systems-performance](../06.6-systems-performance/)（性能分析，从"改对"转向"改快"）
> - [06.7-bpf-observability](../06.7-bpf-observability/)（eBPF 动态追踪，从"调试"转向"可观测"）

---

## ⚠️ 与 05.6 内核调试 / 06.6 性能 / 06.7 可观测的边界

| 维度 | 本模块 (03.6) | 内核调试 (05.6) | 性能之巅 (06.6) | BPF 可观测 (06.7) |
|------|---------------|-----------------|-----------------|-------------------|
| **核心问题** | 用户态代码为什么**坏了** | 内核代码为什么**坏了** | 系统为什么**慢了** | 系统**正在做什么** |
| **问题类型** | crash / 段错误 / 泄漏 / 死锁 / 逻辑错 | crash / Oops / 内核内存泄漏 / 锁死 | 延迟 / 吞吐 / CPU 与 IO 瓶颈 | 事件级追踪 / 热点定位 |
| **视角** | 应用开发者（写业务 → 出 bug → 定位修复） | 内核开发者（写模块 → 出 bug → 定位修复） | 性能工程师 / SRE | 平台工程师 / SRE |
| **工具** | gdb / strace / valgrind / ASan / TSan / perf | printk / kprobes / ftrace / kgdb / KASAN | perf / top / sar / Ftrace | bpftrace / BCC / libbpf |
| **侵入性** | 低 — 无需重编译内核；gdb attach / 插桩 | 高 — 需重编译内核启用调试选项 | 低 — 现有接口 | 极低 — 运行时注入 |
| **时机** | 开发阶段（写完代码调试） | 开发阶段（写内核模块时） | 性能调优阶段 | 生产环境持续观测 |

### 四者关系一句话

> **03.6** 教你"用户态代码坏了怎么修"（correctness，userspace）；
> **05.6** 教你"内核代码坏了怎么修"（correctness，kernel）；
> **06.6** 教你"系统慢了怎么找"（performance）；
> **06.7** 教你"系统在干什么怎么看"（observability）。
>
> 完整链路：先保证正确性（03.6 + 05.6）→ 再优化性能（06.6）→ 最后持续观测（06.7）。

---

## 🔁 用户态 ↔ 内核态调试工具对称表

内核态有一套成熟的调试工具（05.6），用户态几乎一一对应——这正是本模块存在的理由：

| 调试诉求 | 用户态工具 (03.6) | 内核态工具 (05.6) |
|----------|-------------------|-------------------|
| 源码级单步 / 断点 | **gdb** | **kgdb** |
| 观察系统调用序列 | **strace** | **ftrace**（追踪内核函数流） |
| 观察库函数调用 | **ltrace** | —（无直接对应） |
| 内存越界 / 泄漏 / UAF | **valgrind / ASan** | **KASAN / kmemleak / SLUB debug** |
| 数据竞争 | **TSan / helgrind** | **KCSAN** |
| 锁死锁 / 锁序 | **gdb thread + 死锁检测** | **LOCKDEP** |
| 崩溃现场回溯 | **coredump + gdb bt** | **Oops 日志解读** |
| CPU 热点 / cache miss | **perf** | **perf**（同一工具，内核侧视角） |
| 动态探针 | **uprobe / USDT（bpftrace）** | **kprobe / kretprobe** |

---

## 目录结构

```
03.6-userspace-debugging/
├── README.md                  ← 本文件（模块导读）
└── chapter-XX-english-slug/
    ├── README.md              ← 章导读（标题、小节索引、HFT 关联）
    └── notes/                 ← 按主题拆分的笔记
```

与 [05.6-kernel-debugging](../05.6-kernel-debugging/) · [03-linux-userspace-api](../03-linux-userspace-api/) 同一套约定。

---

## 全书章节（8 章）

| 章 | 标题 | 读/跳 | 目录 |
|----|------|-------|------|
| 1 | gdb 基础：断点 / 单步 / 栈 / 变量 | 🔴 精读 | [chapter-01-gdb-basics](./chapter-01-gdb-basics/) |
| 2 | gdb 进阶：多线程调试 / attach / rr 可逆调试 | 🔴 精读 | chapter-02-gdb-advanced |
| 3 | coredump 分析：崩溃现场回溯 | 🔴 精读 | chapter-03-coredump |
| 4 | strace / ltrace：系统调用与库调用追踪 | 🔴 精读 | chapter-04-strace-ltrace |
| 5 | valgrind：内存越界 / 泄漏 / use-after-free | 🔴 精读 | chapter-05-valgrind |
| 6 | sanitizer 家族：ASan / UBSan / TSan | 🔴 精读 | chapter-06-sanitizers |
| 7 | perf 入门：采样 / 火焰图 / cache miss（深交给 06.6） | 选读 | chapter-07-perf-intro |
| 8 | 实战：多线程 + 网络 + 共享内存迷你下单程序全流程调试 | 🔴 精读 | chapter-08-case-study |

> 注：第 2–8 章目录待逐章创建（本模块按需推进）。

---

## 📚 参考书目（辅读，非必读）

> 本模块是**工具链实战**模块，主干是 gdb / strace / valgrind / sanitizer / perf 的官方手册；以下书籍是「排错思路 + 底层原理」的辅读。
>
> ⚠️ **现实提醒**：没有一本「全能的现代 C/C++ 调试大部头」——ASan / UBSan / libFuzzer 都是 2010 年后的工具，几乎无纸质书完整覆盖，资料主要在 LLVM 官方文档、博客、论文。

### 外文原版（两类：①专门调试主题 ②高性能/系统编程书里的调试章节）

**① 专门调试主题：**

| 书目 | 范围 | 评价 |
|------|------|------|
| 《Debugging with GDB》（GNU 官方手册） | 纯用户态、Linux、C/C++；断点 / watchpoint / core dump / 多线程 / 多进程 / gdb 脚本 | 权威，一切 gdb 行为的标准；**免费网页版**；缺点是工具手册，不教分析诡异 bug（野指针、内存破坏） |
| 《Advanced Linux Debugging》⭐ | gdb / core dump / valgrind / 信号 / 栈破坏 / 段错误 / 多线程死锁；少量 ftrace | **最贴合本模块方向**（聚焦 Linux 用户态应用调试），适合嵌入式 Linux、系统 C/C++ |
| 《Writing Solid Code》（旧经典） | 从编码角度避免 bug + 调试思维 + 内存错误分析 | 偏 C、年代老、无 ASan，但排错思维至今有效 |

**② 高性能/系统编程书里的调试章节：**

| 书目 | 调试相关内容 |
|------|-------------|
| 《C++ Concurrency in Action》 | C++ 多线程死锁、数据竞争的调试与排查（质量极高，后续 C++ 多线程会用到） |
| 《High-Performance C++》 / 《Optimized C++》 | 定位内存破坏、崩溃、未定义行为，配合调试工具 |

### 中文补充

| 书目 | 定位 |
|------|------|
| 《C/C++ 代码调试的艺术》 | 国外 gdb 实践整理成中文实操，适合快速上手 |
| 张银奎《软件调试》（第 1 卷） | 底层原理：CPU 异常、断点机制 |

### 学习路径组合（建议）

1. **实操**：GDB 官方手册（网页免费），掌握 gdb 全套命令；
2. **排错思路**：《Advanced Linux Debugging》；
3. **现代工具**：ASan / UBSan 直接看 LLVM 官方文档（书本没有）；
4. **底层原理**：张银奎《软件调试》理解 CPU 异常、断点。

---

## HFT 关联

| 场景 | 相关章节 | 说明 |
|------|----------|------|
| 交易进程段错误崩溃 | Ch1 (gdb) + Ch3 (coredump) | 线上崩溃后加载 core，`bt` 定位崩溃栈帧，反汇编定位精确指令 |
| 多线程竞态导致错单 | Ch2 (gdb 多线程) + Ch6 (TSan) | `thread apply all bt` 看全线程现场；TSan 在开发期抓数据竞争 |
| 内存越界 / 泄漏导致长跑 OOM | Ch5 (valgrind) + Ch6 (ASan) | 交易进程 7×24 运行，慢泄漏是致命伤；ASan 抓越界、valgrind 抓泄漏 |
| 系统调用阻塞 / 多余 syscall | Ch4 (strace) | strace 看 `read`/`recv` 阻塞点、有没有本可避免的 syscall |
| 行情处理 CPU 热点 | Ch7 (perf) | perf 采样定位热点函数，火焰图看调用链（深入交 06.6） |
| 嵌入式板端远程调试 | Ch1–2 (gdbserver) | 树莓派 5 上 gdbserver + 本地 gdb 远程 attach，调试 eBPF/驱动配套用户态程序 |
