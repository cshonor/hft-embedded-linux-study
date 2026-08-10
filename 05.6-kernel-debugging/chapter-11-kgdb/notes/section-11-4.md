# 11.4 断点 / 单步 / 查看变量

> 🔴 精读

## 本节要点

### KGDB 常用 GDB 命令

```gdb
# 断点
(gdb) break schedule              # 函数入口断点
(gdb) break my_driver.c:45        # 源码行断点
(gdb) break my_driver_write+0x20  # 地址偏移断点
(gdb) delete 1                    # 删除断点 1
(gdb) info breakpoints            # 查看断点

# 执行控制
(gdb) continue                    # 继续运行
(gdb) step                        # 单步 (进入函数)
(gdb) next                        # 单步 (跳过函数)
(gdb) finish                      # 运行到函数返回
(gdb) Ctrl+C                      # 中断到 KGDB

# 查看数据
(gdb) print jiffies               # 查看全局变量
(gdb) print *current              # 查看当前进程 task_struct
(gdb) print current->pid          # 查看字段
(gdb) backtrace                   # 栈回溯
(gdb) info registers              # 查看寄存器
(gdb) x/10x 0xffff000012345678    # 查看内存

# 模块调试
(gdb) add-symbol-file my_module.ko 0xffff800000100000
# 加载模块符号 (地址从 /proc/modules 获取)
(gdb) break my_driver_write       # 现在可以设模块断点
```

### KGDB 限制

| 限制 | 说明 |
|------|------|
| 内核暂停 | KGDB 断点暂停整个内核（所有 CPU） |
| 无法查看 sleeping 线程 | 需要切换上下文 |
| 不可调试中断上下文 | KGDB 使用中断通信 |
| 模块符号需手动加载 | `add-symbol-file` |

### 切换线程上下文

```gdb
# 查看所有线程
(gdb) info threads
# 目标机内核线程会显示为不同 thread

# 切换到特定线程
(gdb) thread 2

# 查看 sleeping 线程的栈
(gdb) thread 3
(gdb) backtrace
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** KGDB 设置断点后，内核的哪些 CPU 会暂停？

> 所有 CPU。KGDB 断点触发时发送 IPI (Inter-Processor Interrupt) 给所有 CPU，使其进入 KGDB 暂停状态。这是必要的——断点后内核状态可能不一致，不能让其他 CPU 继续运行。这也是 KGDB 不适合生产环境的原因（暂停整个系统）。

</details>
