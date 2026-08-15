# KDB：内核内置调试器

> 🔴 精读

## 概念详解

### KDB 是什么

KDB 是内核内置的命令行调试器，不需要外部 GDB 连接。直接在目标机控制台上操作，适合快速排查。

### KDB 基本用法

```bash
# 进入 KDB (与 KGDB 相同的入口)
echo g > /proc/sysrq-trigger

# KDB 提示符
kdb>

# 常用命令
kdb> help              # 帮助
kdb> bt                # 当前栈回溯
kdb> btp <pid>         # 指定进程栈回溯
kdb> ps                # 进程列表
kdb> go                # 继续运行
kdb> ss                # 单步
kdb> bp <addr>         # 设置断点
kdb> bc <num>          # 清除断点
kdb> md <addr>         # 查看内存 (dump)
kdb> rd                # 查看寄存器
kdb> dmesg             # 查看内核日志
```

### KDB 常用命令

| 命令 | 功能 | 等价 GDB 命令 |
|------|------|--------------|
| `bt` | 栈回溯 | `backtrace` |
| `btp <pid>` | 指定进程栈 | `thread N` + `bt` |
| `ps` | 进程列表 | `info threads` |
| `go` | 继续运行 | `continue` |
| `ss` | 单步 | `step` |
| `bp <addr>` | 设置断点 | `break` |
| `bc <num>` | 清除断点 | `delete` |
| `md <addr>` | 查看内存 | `x` |
| `rd` | 查看寄存器 | `info registers` |
| `dmesg` | 内核日志 | `log` |
| `cpu <N>` | 切换 CPU | N/A |
| `kgdb` | 切换到 KGDB | N/A |

### KDB vs KGDB

| 特性 | KDB | KGDB |
|------|-----|------|
| 需要 GDB | ❌ | ✅ |
| 源码级调试 | ❌ | ✅ |
| 查看变量 | 有限 | ✅ 完整 |
| 操作便利 | 直接在控制台 | 需要开发机 |
| 适用 | 快速排查 | 深度调试 |
| 速度 | 快（无串口传输） | 慢（串口传输） |

### 切换

```bash
# KGDB 模式下按 Ctrl+C 切到 KDB
# KDB 模式下输入 kgdb 切到 KGDB
kdb> kgdb
# 等待 GDB 连接...
```

### KDB 多 CPU 操作

```bash
kdb> cpu              # 显示当前 CPU
kdb> cpu 2            # 切换到 CPU 2
kdb> rd               # 查看 CPU 2 的寄存器
kdb> bt               # 查看 CPU 2 的栈
```

### HFT 关联应用

```bash
# HFT 快速排查: 系统卡住时用 KDB 查看状态
# 1. 通过串口进入 KDB
#    Alt+SysRq+g 或 echo g > /proc/sysrq-trigger

# 2. 查看所有进程
kdb> ps
# 找到 trade_app 的 PID

# 3. 查看交易进程的栈
kdb> btp 1234
# 查看 trade_app 卡在哪里

# 4. 查看所有 CPU 状态
kdb> cpu 0
kdb> bt
kdb> cpu 1
kdb> bt
kdb> cpu 2
kdb> bt

# 5. 查看内存（检查数据结构）
kdb> md 0xffff000012345678
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** KDB 和 KGDB 如何切换？

> KGDB 模式（GDB 连接中）按 Ctrl+C 中断 GDB 连接，自动切到 KDB。KDB 模式输入 `kgdb` 命令切回 KGDB，等待 GDB 重新连接。切换不需要重启内核。

**Q2:** KDB 的 "go" 命令和 "cpu" 命令分别做什么？

> go：退出 KDB 恢复内核运行。cpu N：切换到 CPU N 的上下文（查看其栈和寄存器）。在多核系统中 KGDB/KDB 默认停在触发 CPU，用 cpu 命令可以检查其他 CPU 状态。

**Q3:** KDB 和 KGDB 在功能上的主要区别？

> KGDB 通过 GDB 提供源码级调试（断点、变量查看、单步），功能强大但需要开发机。KDB 直接在控制台操作，功能有限（查看栈/寄存器/内存）但不需要外部工具，适合快速排查。

**Q4:** KDB 的 `btp <pid>` 命令有什么用？

> 查看指定 PID 进程的栈回溯。不需要切换线程上下文，直接按 PID 查看任意进程的调用栈。适合快速定位某个进程卡在哪里。

**Q5:** HFT 系统卡住时，KDB 比 KGDB 有什么优势？

> KDB 不需要 GDB 连接——通过串口直接操作。系统卡住时，KGDB 可能因为需要 GDB 连接而无法使用（如 GDB 未连接或开发机不可用）。KDB 只需要串口终端，可以立即查看系统状态。

</details>

## 交叉引用

- [05.6 ch11 KGDB 原理与架构](../../chapter-11-kgdb/notes/01-kgdb-architecture.md)
- [05.6 ch11 GDB 连接内核](../../chapter-11-kgdb/notes/03-gdb-connection.md)
- [05.6 ch10 Soft Lockup](../../chapter-10-panic-lockup/notes/02-soft-lockup.md)
