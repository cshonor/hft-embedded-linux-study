# 11.3 GDB 连接内核

> 🔴 精读

## 本节要点

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
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么 `echo g > /proc/sysrq-trigger` 能让内核进入 KGDB？

> SysRq 是内核的"魔术键"机制，通过写入 /proc/sysrq-trigger 触发。`g` 对应 `sysrq_handle_dbg()`，调用 `kgdb_breakpoint()` 在当前位置插入断点指令。内核暂停并等待 GDB 连接（如果已配置 kgdboc）。


**Q:** GDB 连接 KGDB 后，哪些操作是安全的？哪些可能导致系统不稳定？

> 安全：查看变量/寄存器、单步执行、设置断点、查看调用栈。不安全：(1) 修改全局变量可能破坏一致性；(2) 长时间暂停导致硬件看门狗触发；(3) 在中断上下文设断点可能死锁。建议尽量只读分析。

</details>

## 交叉引用

- [05.6 ch11 breakpoints](chapter-11-kgdb/notes/section-11-4.md)
