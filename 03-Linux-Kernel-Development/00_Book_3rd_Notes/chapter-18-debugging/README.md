# Ch 18 调试 · Debugging

> **Linux Kernel Development 3rd** · Robert Love · **选读**

> 本章定位：内核 **缺 gdb 便利、小错即崩** — `printk`、**Oops**、**BUG_ON**、SysRq、kgdb、探测技巧、**git bisect**。写驱动/改内核前的 **救命工具箱**。

---

## 本节结构

| 节 | 主题 | 带走什么 |
|----|------|----------|
| **① printk** | 打印调试 | 日志级 · 环形缓冲 |
| **② Oops** | 严重错误 | panic vs 杀进程 |
| **③ 编译选项** | Kernel Hacking | sleep-in-spinlock 检测 |
| **④ 断言与栈** | BUG / panic | `dump_stack` |
| **⑤ SysRq** | 魔法键 | s-u-b 救命 |
| **⑥ 调试器** | gdb / kgdb | kcore 只读 |
| **⑦ 探测技巧** | UID / ratelimit | 二分 Git |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 为何内核调试更难 | [notes/section-18.1-为何内核调试更难.md](./notes/section-18.1-为何内核调试更难.md) |
| 通过打印调试 | [notes/section-18.2-通过打印调试.md](./notes/section-18.2-通过打印调试.md) |
| Oops | [notes/section-18.3-Oops.md](./notes/section-18.3-Oops.md) |
| 内核调试选项 | [notes/section-18.4-内核调试选项.md](./notes/section-18.4-内核调试选项.md) |
| 引发 Bug 与打印信息 | [notes/section-18.5-引发-Bug-与打印信息.md](./notes/section-18.5-引发-Bug-与打印信息.md) |
| 神奇的 SysRq 键 | [notes/section-18.6-神奇的-SysRq-键.md](./notes/section-18.6-神奇的-SysRq-键.md) |
| 内核调试器 | [notes/section-18.7-内核调试器.md](./notes/section-18.7-内核调试器.md) |
| 探测系统 | [notes/section-18.8-探测系统.md](./notes/section-18.8-探测系统.md) |
| 二分法查找 | [notes/section-18.9-二分法查找.md](./notes/section-18.9-二分法查找.md) |

---

## 本章小结

| 工具/概念 | 何时用 |
|-----------|--------|
| **`printk` + loglevel** | 第一手、任意上下文 |
| **环形 log buffer** | `dmesg` / journal |
| **Oops + kallsyms** | 崩溃后读栈 |
| **BUG_ON / panic** | 断言 / 致命停 |
| **`dump_stack`** | 非致命路径跟踪 |
| **SysRq s-u-b** | 死机前尽量落盘 |
| **kgdb** | 全功能远程调试 |
| **`printk_ratelimit`** | 高频路径 |
| **`git bisect`** | 回归哪次提交 |

---

## 本章学习目标 · 自检

- [ ] 说出 **`printk` 在中断里可用** 但 **`malloc(GFP_KERNEL)` 不可用**
- [ ] 区分 **Oops 杀进程** vs **panic 挂机**
- [ ] 解释 **`kallsyms`** 为何能直接读符号栈
- [ ] 背 **SysRq s / u / b** 含义与顺序
- [ ] 知 **`dump_stack` vs BUG_ON`** 区别
- [ ] 会用 **`git bisect`** 概念流程

---

## 相关章节

- 上一章：[../chapter-17-devices-modules/](../chapter-17-devices-modules/)
- 下一章：[../chapter-19-portability/](../chapter-19-portability/)
- 全书导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
