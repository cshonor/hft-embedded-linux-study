# 11.1 KGDB 原理与架构

> 🔴 精读 · Part 3: Diagnostics & Advanced Tools

## 本节要点

### KGDB 是什么

KGDB 是内核的 GDB stub——允许用 GDB 远程调试运行中的内核，支持断点、单步、查看变量。

### 架构

```
开发机 (GDB)              目标机 (内核)
┌──────────┐              ┌──────────────┐
│ gdb      │ ──串口/网口──→│ KGDB stub    │
│ vmlinux  │              │ (内核中)      │
│ .ko 文件  │              │              │
└──────────┘              │ 断点/单步/    │
                          │ 查看变量      │
                          └──────────────┘
```

### 启用 KGDB

```bash
# 内核配置
CONFIG_KGDB=y
CONFIG_KGDB_SERIAL_CONSOLE=y
CONFIG_DEBUG_INFO=y
CONFIG_GDB_SCRIPTS=y

# boot 参数
# kgdboc=ttyAMA0,115200  — KGDB over serial console (树莓派 UART)
# kgdbwait               — 启动时等待 GDB 连接
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** KGDB 和 KDB 的区别是什么？

> KGDB 是 GDB 远程调试后端，需要开发机上的 GDB 连接。KDB 是内核内置的调试器，直接在目标机控制台上操作（无需 GDB）。KGDB 功能更强大（源码级、变量查看），KDB 更方便（无需第二台机器）。可以切换：在 KGDB 模式下按 Ctrl+C 切到 KDB。


**Q:** KGDB 和 KDB 的区别是什么？

> KGDB 需要外部 GDB 连接（通过串口/网络），提供源码级调试。KDB 是内建的命令行调试器（不需要 GDB），在内核控制台直接输入命令。KDB 功能比 GDB 少但无需外部连接，适合无 GDB 环境的快速诊断。可以 `echo 1 > /sys/module/kgdboc/parameters/kgdbcon` 在两者间切换。

</details>

## 交叉引用

- [05.6 ch11 KDB](chapter-11-kgdb/notes/section-11-6.md)
