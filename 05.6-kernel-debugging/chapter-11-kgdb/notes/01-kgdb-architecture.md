# KGDB 原理与架构

> 🔴 精读 · Part 3: Diagnostics & Advanced Tools

## 概念详解

### KGDB 是什么

KGDB 是内核的 GDB stub——允许用 GDB 远程调试运行中的内核，支持断点、单步、查看变量。相当于内核版的 `gdbserver`。

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

通信方式:
  - 串口 (UART): kgdboc=ttyAMA0,115200
  - 网络: kgdboe (KGDB over Ethernet)
  - USB: kgdboc=ttyUSB0
```

### 启用 KGDB

```bash
# 内核配置
CONFIG_KGDB=y
CONFIG_KGDB_SERIAL_CONSOLE=y   # 串口通信
CONFIG_KGDB_KDB=y              # KDB 内置调试器
CONFIG_DEBUG_INFO=y            # 调试符号
CONFIG_GDB_SCRIPTS=y           # GDB 辅助脚本

# boot 参数
# kgdboc=ttyAMA0,115200  — KGDB over serial console
# kgdbwait               — 启动时等待 GDB 连接
# nokaslr                — 禁用 KASLR (简化调试)
```

### KGDB vs KGDBOC

| 特性 | KGDB | KGDBOC |
|------|------|--------|
| 全称 | Kernel GDB | KGDB Over Console |
| 功能 | GDB 远程调试后端 | 复用控制台串口做 KGDB |
| 需要 | 串口/网络 | 控制台串口 |
| 配置 | `kgdboc=ttyAMA0,115200` | 同左 |
| 切换 | SysRq+g | SysRq+g |

### KGDB 工作流程

```
1. 内核启动时加载 KGDB (kgdboc=ttyAMA0,115200)
2. 运行中通过 SysRq+g 触发 KGDB
   → 内核暂停所有 CPU
   → KGDB stub 等待 GDB 连接
3. 开发机 GDB 连接
   → (gdb) target remote /dev/ttyUSB0
4. GDB 命令控制内核
   → 断点/单步/查看变量
5. continue 恢复内核运行
```

### KGDB vs KDB

| 特性 | KGDB | KDB |
|------|------|-----|
| 需要 GDB | ✅ | ❌ |
| 源码级调试 | ✅ | ❌ |
| 查看变量 | 完整 | 有限 |
| 操作便利 | 需要开发机 | 直接在控制台 |
| 适用 | 深度调试 | 快速排查 |
| 切换 | Ctrl+C → KDB | `kgdb` → KGDB |

### HFT 关联应用

KGDB 适合 HFT 内核模块的开发阶段调试：

```bash
# HFT 模块调试流程
# 1. 在 QEMU 或树莓派上启动带 KGDB 的内核
# 2. 加载 HFT 模块
# 3. 通过 SysRq+g 进入 KGDB
# 4. GDB 连接，设断点在交易函数
# 5. 触发交易，断点命中
# 6. 查看变量、调用栈、寄存器
# 7. 单步执行，分析逻辑
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** KGDB 和 KDB 的区别是什么？

> KGDB 是 GDB 远程调试后端，需要开发机上的 GDB 连接。KDB 是内核内置的调试器，直接在目标机控制台上操作（无需 GDB）。KGDB 功能更强大（源码级、变量查看），KDB 更方便（无需第二台机器）。可以切换：在 KGDB 模式下按 Ctrl+C 切到 KDB。

**Q2:** KGDB 需要哪些内核配置？

> `CONFIG_KGDB=y`（KGDB 核心）、`CONFIG_KGDB_SERIAL_CONSOLE=y`（串口通信）、`CONFIG_DEBUG_INFO=y`（调试符号）、`CONFIG_GDB_SCRIPTS=y`（辅助脚本）。boot 参数 `kgdboc=ttyAMA0,115200` 指定串口。

**Q3:** KGDB 支持哪些通信方式？

> 串口（最常用，`kgdboc=ttyAMA0,115200`）、网络（`kgdboe`，需要网络栈工作）、USB 串口（`kgdboc=ttyUSB0`）。HFT 开发用网络（快），生产崩溃分析用串口（可靠）。

**Q4:** KGDB 和 QEMU 内置 GDB stub 有什么区别？

> KGDB 是内核内的 GDB stub，需要内核配置和运行在目标机上。QEMU GDB stub 是 QEMU 提供的，不需要内核支持，可以从第一条指令调试。QEMU 更适合开发期（不需要 KGDB 配置），KGDB 更适合真实硬件调试。

**Q5:** HFT 模块调试为什么推荐用 KGDB 而非仅用 printk？

> KGDB 提供交互式调试——可以设断点、查看变量、单步执行，精确定位问题。printk 只能输出信息，需要反复修改代码重新编译。KGDB 适合复杂逻辑调试，printk 适合简单信息收集。两者互补。

</details>

## 交叉引用

- [05.6 ch11 串口配置](chapter-11-kgdb/notes/02-uart-setup.md)
- [05.6 ch11 GDB 连接内核](chapter-11-kgdb/notes/03-gdb-connection.md)
- [05.6 ch11 KDB 内置调试器](chapter-11-kgdb/notes/06-kdb-builtin-debugger.md)
- [05.6 ch11 QEMU + KGDB](chapter-11-kgdb/notes/07-qemu-kgdb.md)
