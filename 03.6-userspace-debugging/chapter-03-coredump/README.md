# Ch3 coredump 分析：崩溃现场回溯

> 🔴 精读 · 线上崩溃的第一现场

进程崩溃（段错误、断言、SIGBUS 等）时，内核可以把它的**内存快照**写成 core 文件。这是「事后破案」最关键的证据——不用复现、不用打断运行，直接对 core 做尸检。本章覆盖：怎么让 core 生成出来、怎么加载 core 回溯调用栈、怎么深入分析被破坏的内存。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 3.1 core 文件生成配置（ulimit -c / core_pattern / systemd-coredump） | `notes/01-core-dump-config.md` |
| 3.2 加载 core 回溯（gdb prog core / bt / frame / 现场还原） | `notes/02-load-core-backtrace.md` |
| 3.3 深入内存分析（x 看内存 / 多线程 core / 反汇编 / 与 rr 互补） | `notes/03-analyze-corrupted-memory.md` |

---

## HFT 关联

精读。交易进程 7×24 运行，崩溃后第一动作就是**加载 core 做尸检**——`bt` 定位崩溃函数、`info locals` 看崩溃点变量、`x` 看被破坏的数据结构。core 是「崩溃瞬间的全量快照」，配合 gdb 三章（Ch1 基本功 + Ch2 多线程/attach）形成完整的事后分析闭环。
