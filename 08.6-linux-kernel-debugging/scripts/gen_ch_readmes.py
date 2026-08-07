import os

BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

chapters = [
    {
        "dir": "chapter-01-introduction",
        "num": 1,
        "title": "A General Introduction to Debugging Software",
        "part": "Part 1: Introduction & Approaches",
        "read": "skip",
        "summary": "调试的历史背景与经典案例（Patriot 导弹、Ariane 5 火箭、Mars Pathfinder、Boeing 737 MAX），以及开发环境搭建。",
        "sections": [
            "1.1 软件调试的起源与误区",
            "1.2 经典 Bug 案例",
            "1.3 开发环境搭建（VM / 原生 Linux）",
            "1.4 生产内核 vs 调试内核",
        ],
        "hft": "了解即可。Bug 案例有助于建立对调试重要性的认知，但与 HFT 无直接关联。",
    },
    {
        "dir": "chapter-02-approaches",
        "num": 2,
        "title": "Approaches to Kernel Debugging",
        "part": "Part 1: Introduction & Approaches",
        "read": "skip",
        "summary": "内核调试方法论概览：仪表化 (instrumentation) vs 分析 (analysis)，以及何时用哪种方法。",
        "sections": [
            "2.1 内核调试的挑战",
            "2.2 仪表化方法概览",
            "2.3 分析方法概览",
            "2.4 工具选择决策树",
        ],
        "hft": "快速翻阅。决策树在后续章节用到具体工具时可回来参考。",
    },
    {
        "dir": "chapter-03-printk",
        "num": 3,
        "title": "Debug via Instrumentation - printk and Friends",
        "part": "Part 2: Instrumentation & Memory Debugging",
        "read": "deep",
        "summary": "printk 体系：日志级别、速率限制、dynamic debug 框架 (pr_debug / dev_dbg 动态开关)、ftrace_printk。",
        "sections": [
            "3.1 printk 基础与日志级别",
            "3.2 速率限制与异步打印",
            "3.3 dynamic debug 框架",
            "3.4 dev_dbg 与设备相关调试",
            "3.5 ftrace_printk (trace_marker 前身)",
        ],
        "hft": "精读。dynamic debug 框架在 6.x 内核中仍是主力调试手段，可在不重编译的情况下动态开关调试输出。HFT 自定义内核模块应大量使用 pr_debug。",
    },
    {
        "dir": "chapter-04-kprobes",
        "num": 4,
        "title": "Debug via Instrumentation - Kprobes",
        "part": "Part 2: Instrumentation & Memory Debugging",
        "read": "deep",
        "summary": "Kprobes 框架：kprobe (入口探针) / kretprobe (返回探针) / jprobe (已弃用)；静态注册 vs 动态注册；perf probe 和 bpftrace 的底层。",
        "sections": [
            "4.1 Kprobes 原理与架构",
            "4.2 kprobe：函数入口探针",
            "4.3 kretprobe：函数返回探针",
            "4.4 动态注册 Kprobes (通过 /sys)",
            "4.5 perf probe 与 Kprobes 的关系",
            "4.6 Kprobes 与 eBPF 的关系",
        ],
        "hft": "精读。Kprobes 是 HFT 延迟溯源的核心工具之一，可在生产环境动态插入探针测量内核函数耗时。同时也是 eBPF tracing 的底层机制。",
    },
    {
        "dir": "chapter-05-memory-debug-1",
        "num": 5,
        "title": "Debugging Kernel Memory Issues - Part 1",
        "part": "Part 2: Instrumentation & Memory Debugging",
        "read": "deep",
        "summary": "内核内存错误检测：KASAN (地址消毒器)、UBSAN (未定义行为消毒器)、SLUB debug (slab 泄漏检测)、kmemleak (内核内存泄漏检测)。",
        "sections": [
            "5.1 内核内存错误的类型",
            "5.2 KASAN：地址消毒器 (越界 / use-after-free)",
            "5.3 UBSAN：未定义行为检测",
            "5.4 SLUB debug：slab 分配器调试",
            "5.5 kmemleak：内核内存泄漏检测",
        ],
        "hft": "精读。写自定义内核模块时，KASAN 能在开发期捕获 90% 的内存错误。树莓派 5 ARM64 完整支持 KASAN，需 CONFIG_KASAN=y 重编译内核。",
    },
    {
        "dir": "chapter-06-memory-debug-2",
        "num": 6,
        "title": "Debugging Kernel Memory Issues - Part 2",
        "part": "Part 2: Instrumentation & Memory Debugging",
        "read": "skip",
        "summary": "KFENCE (Kernel Electric Fence) 等较新的内存调试特性，以及内存调试工具的组合使用策略。",
        "sections": [
            "6.1 KFENCE：轻量级内存错误检测",
            "6.2 内存调试工具组合策略",
            "6.3 生产环境内存监控",
        ],
        "hft": "按需查阅。KFENCE 是 5.x 引入的低开销内存检测器，可在生产环境长期开启。需要时再回来读。",
    },
    {
        "dir": "chapter-07-oops",
        "num": 7,
        "title": "Oops! Interpreting the Kernel Bug Diagnostic",
        "part": "Part 3: Diagnostics & Advanced Tools",
        "read": "deep",
        "summary": "Oops 日志深度解读：寄存器转储、栈回溯、Call Trace 分析、addr2line / objdump 定位源码行、panic vs oops 区别。",
        "sections": [
            "7.1 Oops 是什么 / panic vs oops",
            "7.2 寄存器转储解读",
            "7.3 栈回溯 (Call Trace) 分析",
            "7.4 addr2line 定位源码行",
            "7.5 objdump 反汇编辅助分析",
            "7.6 模块 Oops 的特殊处理",
        ],
        "hft": "精读。内核模块崩溃时的第一现场就是 Oops 日志。能快速解读 Call Trace 并用 addr2line 定位到源码行，是内核开发者的核心技能。",
    },
    {
        "dir": "chapter-08-lock-debug",
        "num": 8,
        "title": "Lock Debugging",
        "part": "Part 3: Diagnostics & Advanced Tools",
        "read": "deep",
        "summary": "锁调试：LOCKDEP (锁依赖检测器，发现死锁/锁序问题)、KCSAN (并发消毒器，检测数据竞争)、lockdep 的 lock_stat 统计。",
        "sections": [
            "8.1 并发 bug 的类型：死锁 / 活锁 / 数据竞争",
            "8.2 LOCKDEP：锁依赖检测器",
            "8.3 用 LOCKDEP 发现潜在死锁",
            "8.4 lock_stat：锁竞争统计",
            "8.5 KCSAN：数据竞争检测器",
            "8.6 在树莓派上启用 LOCKDEP / KCSAN",
        ],
        "hft": "精读。并发 bug 是内核中最难调的。LOCKDEP 在开发期能发现锁序问题，KCSAN 能检测无锁变量的数据竞争。HFT 自定义内核模块必须用 LOCKDEP 验证。",
    },
    {
        "dir": "chapter-09-ftrace",
        "num": 9,
        "title": "Tracing the Kernel Flow",
        "part": "Part 3: Diagnostics & Advanced Tools",
        "read": "deep",
        "summary": "Ftrace 体系：tracefs 接口、函数追踪 / 函数图追踪、事件追踪、trace-cmd 命令行前端、KernelShark GUI 前端、perf-tools ftrace wrapper。",
        "sections": [
            "9.1 Ftrace 架构与 tracefs 接口",
            "9.2 函数追踪 (function tracer)",
            "9.3 函数图追踪 (function_graph tracer)",
            "9.4 事件追踪 (trace events)",
            "9.5 trace-cmd：命令行前端",
            "9.6 KernelShark：GUI 前端",
            "9.7 perf-tools ftrace wrapper",
            "9.8 Ftrace 与 eBPF 的关系",
        ],
        "hft": "精读。Ftrace 是 HFT 延迟分析的关键工具链。与 19-systems-performance Ch14 (Ftrace) 互补：本书侧重调试视角，19 侧重性能视角。",
    },
    {
        "dir": "chapter-10-panic-lockup",
        "num": 10,
        "title": "Kernel Panic, Lockups, and Hangs",
        "part": "Part 3: Diagnostics & Advanced Tools",
        "read": "deep",
        "summary": "内核挂死诊断：soft lockup (CPU 长时间不调度) / hard lockup (CPU 不响应中断) / hangcheck timer / watchdog 机制 / 自定义 panic handler。",
        "sections": [
            "10.1 Kernel Panic 的触发与处理",
            "10.2 Soft Lockup：CPU 长时间不调度",
            "10.3 Hard Lockup：CPU 不响应中断",
            "10.4 Watchdog 机制详解",
            "10.5 Hangcheck Timer",
            "10.6 自定义 Panic Handler",
            "10.7 Kdump / Kexec 崩溃转储",
        ],
        "hft": "精读。HFT 系统挂死时的第一诊断手段。soft lockup 通常意味着内核模块中有死循环或自旋锁持有过久。",
    },
    {
        "dir": "chapter-11-kgdb",
        "num": 11,
        "title": "Using Kernel GDB (KGDB)",
        "part": "Part 3: Diagnostics & Advanced Tools",
        "read": "deep",
        "summary": "KGDB 源码级调试：串口配置、断点 / 单步 / 查看变量、调试内核模块、树莓派 UART 配置、KGDB + KDB (内核调试器) 组合使用。",
        "sections": [
            "11.1 KGDB 原理与架构",
            "11.2 串口配置（含树莓派 UART）",
            "11.3 GDB 连接内核",
            "11.4 断点 / 单步 / 查看变量",
            "11.5 调试内核模块 (loadable module)",
            "11.6 KDB：内核内置调试器",
            "11.7 KGDB 与 QEMU 虚拟机调试",
        ],
        "hft": "精读。写内核模块时最高效的调试方式。树莓派 5 需通过 GPIO 14/15 (UART) 连接串口调试线。",
    },
    {
        "dir": "chapter-12-misc",
        "num": 12,
        "title": "A Few More Kernel Debugging Approaches",
        "part": "Part 3: Diagnostics & Advanced Tools",
        "read": "skip",
        "summary": "代码覆盖率工具 (GCOV / KCOV)、内核测试框架、syzkaller 模糊测试、静态分析工具。",
        "sections": [
            "12.1 GCOV / KCOV 代码覆盖率",
            "12.2 内核测试框架 (kselftest / KUnit)",
            "12.3 syzkaller 模糊测试",
            "12.4 静态分析工具 (Smatch / Sparse)",
        ],
        "hft": "按需查阅。做内核模块测试时回来参考。KUnit 适合单元测试自写模块。",
    },
]

for ch in chapters:
    readme_path = os.path.join(BASE, ch["dir"], "README.md")
    read_label = "🔴 精读" if ch["read"] == "deep" else "⬜ 跳读"

    lines = []
    lines.append("# Ch{} {}".format(ch["num"], ch["title"]))
    lines.append("")
    lines.append("> {} · {}".format(ch["part"], read_label))
    lines.append("")
    lines.append(ch["summary"])
    lines.append("")
    lines.append("---")
    lines.append("")
    lines.append("## 小节索引")
    lines.append("")
    lines.append("| 小节 | 笔记文件 |")
    lines.append("|------|----------|")
    for sec in ch["sections"]:
        slug = sec.split(" ", 1)[0].replace(".", "-")
        lines.append("| {} | `notes/section-{}.md` |".format(sec, slug))
    lines.append("")
    lines.append("---")
    lines.append("")
    lines.append("## HFT 关联")
    lines.append("")
    lines.append(ch["hft"])
    lines.append("")

    with open(readme_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))
    print("OK: {}".format(readme_path))
