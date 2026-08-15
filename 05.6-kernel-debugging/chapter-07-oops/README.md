# Ch7 Oops! Interpreting the Kernel Bug Diagnostic

> Part 3: Diagnostics & Advanced Tools · 🔴 精读

Oops 日志深度解读：寄存器转储、栈回溯、Call Trace 分析、addr2line / objdump 定位源码行、panic vs oops 区别。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 7.1 Oops 是什么 / panic vs oops | `notes/01-oops-vs-panic.md` |
| 7.2 寄存器转储解读 | `notes/02-register-dump.md` |
| 7.3 栈回溯 (Call Trace) 分析 | `notes/03-call-trace-analysis.md` |
| 7.4 addr2line 定位源码行 | `notes/04-addr2line.md` |
| 7.5 objdump 反汇编辅助分析 | `notes/05-objdump-disassembly.md` |
| 7.6 模块 Oops 的特殊处理 | `notes/06-module-oops.md` |

---

## HFT 关联

精读。内核模块崩溃时的第一现场就是 Oops 日志。能快速解读 Call Trace 并用 addr2line 定位到源码行，是内核开发者的核心技能。
