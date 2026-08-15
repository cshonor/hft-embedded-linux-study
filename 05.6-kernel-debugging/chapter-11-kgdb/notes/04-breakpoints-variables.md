# 断点 / 单步 / 查看变量

> 🔴 精读

## 概念详解

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
(gdb) break my_driver_write       # 现在可以设模块断点
```

### 硬件断点 vs 软件断点

| 类型 | 原理 | 数量 | 速度 | 适用 |
|------|------|------|------|------|
| 硬件断点 | CPU 调试寄存器 | 有限 (ARM64: 4 BP + 4 WP) | 快 | 高频函数/只读内存 |
| 软件断点 | 替换指令为 BRK | 无限 | 略慢 | 普通函数 |

```gdb
# 硬件断点
(gdb) hbreak schedule            # 硬件断点
(gdb) rwatch my_var              # 读 watchpoint
(gdb) watch my_var               # 写 watchpoint
(gdb) awatch my_var              # 读写 watchpoint
```

### 条件断点

```gdb
# 条件断点: 只在条件满足时暂停
(gdb) break schedule if current->pid == 1234
# 只在 PID 1234 调用 schedule 时断

# 注意: 条件断点每次命中都暂停内核进 KGDB 检查条件
# 高频函数设条件断点会导致系统几乎停顿!
# 替代方案: 用 kprobe + filter (内核侧过滤)
```

### 切换线程上下文

```gdb
# 查看所有线程
(gdb) info threads

# 切换到特定线程
(gdb) thread 2

# 查看 sleeping 线程的栈
(gdb) thread 3
(gdb) backtrace
```

### KGDB 限制

| 限制 | 说明 | 解决方案 |
|------|------|---------|
| 内核暂停 | 断点暂停整个内核（所有 CPU） | 无法避免 |
| 无法查看 sleeping 线程 | 需要切换上下文 | `thread N` + `bt` |
| 不可调试中断上下文 | KGDB 使用中断通信 | 用 kprobe 替代 |
| 模块符号需手动加载 | `add-symbol-file` | 用 gdb 脚本自动化 |
| 条件断点极慢 | 每次命中都暂停 | 用 kprobe + filter |
| 看门狗可能触发 | 长时间暂停 | 暂停看门狗或用短操作 |

### HFT 关联应用

```gdb
# HFT 模块调试: 分析交易延迟
(gdb) break on_trade_signal
(gdb) continue
# ... 断点命中 ...
(gdb) print order->price          # 查看订单价格
(gdb) print order->quantity       # 查看订单数量
(gdb) bt                          # 查看调用链
(gdb) info registers              # 查看寄存器

# 分析锁竞争
(gdb) break spin_lock
(gdb) continue
# ... 锁命中 ...
(gdb) bt                          # 谁在获取锁
(gdb) info threads                # 其他线程状态
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** KGDB 设置断点后，内核的哪些 CPU 会暂停？

> 所有 CPU。KGDB 断点触发时发送 IPI 给所有 CPU，使其进入 KGDB 暂停状态。这是必要的——断点后内核状态可能不一致，不能让其他 CPU 继续运行。这也是 KGDB 不适合生产环境的原因。

**Q2:** KGDB 中的硬件断点和软件断点有什么区别？

> 硬件断点用 CPU 调试寄存器（ARM64: 4 个 watchpoint + 4 个 breakpoint），不修改代码，速度快但数量有限。软件断点替换指令为 BRK，数量无限但修改代码可能被 icache 缓存问题影响。KGDB 优先用硬件断点。

**Q3:** KGDB 条件断点如何工作？有什么限制？

> KGDB 条件断点在断点命中后由 GDB 检查条件——每次命中都暂停内核进 KGDB 检查，开销极大。高频函数设条件断点会导致系统几乎停顿。替代方案：用 kprobe + filter（内核侧过滤，不需要暂停）。

**Q4:** 为什么 KGDB 不能调试中断上下文？

> KGDB 使用中断（串口中断或网络中断）与 GDB 通信。如果在中断上下文中断，KGDB 无法再使用中断通信（中断不可重入）。替代方案：用 kprobe 在中断上下文中收集信息，或用 ftrace 追踪中断处理。

**Q5:** HFT 模块调试中如何查看 sleeping 线程的状态？

> (1) `info threads` 查看所有线程；(2) `thread N` 切换到目标线程；(3) `backtrace` 查看该线程的调用栈。通过这种方式可以查看哪些线程在等锁、哪些在睡眠。

</details>

## 交叉引用

- [05.6 ch11 GDB 连接内核](../../chapter-11-kgdb/notes/03-gdb-connection.md)
- [05.6 ch11 调试内核模块](../../chapter-11-kgdb/notes/05-module-debugging.md)
- [05.6 ch04 kprobes](../../chapter-04-kprobes/notes/02-kprobe-entry-handler.md)
