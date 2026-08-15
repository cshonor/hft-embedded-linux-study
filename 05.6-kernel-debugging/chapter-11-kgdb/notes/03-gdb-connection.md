# GDB 连接内核

> 🔴 精读

## 概念详解

### 连接流程

```bash
# 1. 目标机: 进入 KGDB 模式
# 方法 A: 启动时等待 (kgdbwait 参数)
# 方法 B: 运行时触发
echo g > /proc/sysrq-trigger  # SysRq + g = 进入 KGDB

# 方法 C: panic 时自动进入 (需配置)

# 2. 开发机: 启动 GDB
aarch64-linux-gnu-gdb vmlinux

# 3. 连接
(gdb) target remote /dev/ttyUSB0
# 或网络
(gdb) target remote 192.168.1.50:5555

# 4. 连接成功后可以:
(gdb) continue          # 继续运行
(gdb) Ctrl+C            # 中断到 KGDB
(gdb) break schedule    # 设置断点
(gdb) continue          # 继续, 等断点命中
```

### SysRq 触发

```bash
# SysRq 组合键 (在串口终端)
# Alt+SysRq+g  — 进入 KGDB
# echo g > /proc/sysrq-trigger  — 命令行方式

# 其他有用的 SysRq:
# echo t > /proc/sysrq-trigger  — 打印所有任务栈
# echo w > /proc/sysrq-trigger  — 打印阻塞任务
# echo c > /proc/sysrq-trigger  — 触发 panic (测试 kdump)
# echo p > /proc/sysrq-trigger  — 打印当前 CPU 寄存器
# echo l > /proc/sysrq-trigger  — 打印所有 CPU 栈
```

### SysRq 常用命令

| 键 | 功能 | 用途 |
|----|------|------|
| `g` | 进入 KGDB | 调试 |
| `t` | 打印所有任务栈 | 查看系统状态 |
| `w` | 打印阻塞任务 | 查看死锁 |
| `c` | 触发 panic | 测试 kdump |
| `p` | 打印 CPU 寄存器 | 调试 |
| `l` | 打印所有 CPU 栈 | 全局状态 |
| `b` | 立即重启 | 紧急重启 |
| `o` | 立即关机 | 紧急关机 |

### GDB 连接后的操作

```gdb
# 基本操作
(gdb) continue              # 继续运行
(gdb) Ctrl+C                # 中断到 KGDB
(gdb) detach                # 断开连接（内核继续运行）

# 查看信息
(gdb) backtrace             # 栈回溯
(gdb) info registers        # 寄存器
(gdb) info threads          # 所有线程
(gdb) print jiffies         # 全局变量
(gdb) print *current        # 当前进程
(gdb) print current->pid    # 字段访问

# 断点
(gdb) break schedule        # 函数断点
(gdb) break my_driver.c:45  # 源码行断点
(gdb) info breakpoints      # 查看断点
(gdb) delete 1              # 删除断点

# 单步
(gdb) step                  # 单步进入
(gdb) next                  # 单步跳过
(gdb) finish                # 运行到返回
```

### KGDB 模式下的限制

| 操作 | 安全 | 说明 |
|------|------|------|
| 查看变量 | ✅ | 只读 |
| 查看寄存器 | ✅ | 只读 |
| 查看调用栈 | ✅ | 只读 |
| 设置断点 | ✅ | |
| 单步执行 | ⚠️ | 可能影响时序 |
| 修改变量 | ⚠️ | 可能破坏一致性 |
| 长时间暂停 | ⚠️ | 硬件看门狗可能触发 |

### HFT 关联应用

```bash
# HFT 模块调试典型流程
# 1. 加载模块
insmod my_hft_module.ko

# 2. 进入 KGDB
echo g > /proc/sysrq-trigger

# 3. GDB 连接
aarch64-linux-gnu-gdb vmlinux
(gdb) target remote /dev/ttyUSB0

# 4. 加载模块符号
(gdb) add-symbol-file my_hft_module.ko 0xffff800000100000

# 5. 设断点在交易函数
(gdb) break on_trade_signal
(gdb) continue

# 6. 触发交易，断点命中
# 7. 查看变量和调用栈
(gdb) bt
(gdb) print *order
(gdb) print order->price
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么 `echo g > /proc/sysrq-trigger` 能让内核进入 KGDB？

> SysRq 是内核的"魔术键"机制。`g` 对应 `sysrq_handle_dbg()`，调用 `kgdb_breakpoint()` 在当前位置插入断点指令。内核暂停并等待 GDB 连接（如果已配置 kgdboc）。

**Q2:** GDB 连接 KGDB 后，哪些操作是安全的？

> 安全：查看变量/寄存器、单步执行、设置断点、查看调用栈。不安全：(1) 修改全局变量可能破坏一致性；(2) 长时间暂停导致硬件看门狗触发；(3) 在中断上下文设断点可能死锁。建议尽量只读分析。

**Q3:** SysRq 的 `t` 命令有什么用？

> 打印所有任务的调用栈。在调试死锁或系统卡住时非常有用——可以看到每个进程在哪个函数中阻塞。不需要 GDB 连接，直接在串口终端执行。

**Q4:** GDB 的 `detach` 和 `continue` 有什么区别？

> `continue` 让内核继续运行，GDB 保持连接（可以在后续 Ctrl+C 再次中断）。`detach` 断开 GDB 与 KGDB 的连接，内核继续运行且不再等待 GDB。调试结束用 `detach`。

**Q5:** HFT 模块调试时为什么要先加载模块符号再设断点？

> 模块加载到动态地址。如果不加载符号，GDB 不知道模块函数的地址，无法设置断点。`add-symbol-file` 告诉 GDB 模块各段的实际地址，之后才能按函数名/源码行设断点。

</details>

## 交叉引用

- [05.6 ch11 KGDB 原理与架构](chapter-11-kgdb/notes/01-kgdb-architecture.md)
- [05.6 ch11 串口配置](chapter-11-kgdb/notes/02-uart-setup.md)
- [05.6 ch11 断点/单步/查看变量](chapter-11-kgdb/notes/04-breakpoints-variables.md)
- [05.6 ch11 调试内核模块](chapter-11-kgdb/notes/05-module-debugging.md)
