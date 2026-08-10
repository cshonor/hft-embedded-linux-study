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

</details>
