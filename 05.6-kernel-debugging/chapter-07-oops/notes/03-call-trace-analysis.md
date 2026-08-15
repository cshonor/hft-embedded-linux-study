# 栈回溯 (Call Trace) 分析

> 🔴 精读

## 概念详解

### Call Trace 是什么

Call Trace（调用栈回溯）是 Oops 日志中最关键的信息之一。它记录了从崩溃点到系统入口的完整调用链，帮助开发者理解"代码是怎么执行到崩溃点的"。

### ARM64 栈回溯原理

ARM64 使用 `x29`（帧指针 FP）和 `x30`（链接寄存器 LR）进行栈回溯：

```
栈帧结构 (每个函数调用创建一个栈帧):
  ┌──────────────────┐ ← 高地址
  │   上一个 FP       │ ← x29 指向这里
  │   返回地址 (LR)    │
  │   局部变量         │
  └──────────────────┘ ← 低地址 (下一个栈帧的 FP)
```

回溯过程：从当前 FP 开始，逐帧读取上一帧的 FP 和 LR，直到栈底。

### Call Trace 示例

```
[  123.456900] Call trace:
[  123.456905]  my_driver_write+0x3c/0x100 [my_module]
[  123.456910]  vfs_write+0xf4/0x2b0
[  123.456915]  ksys_write+0x74/0x100
[  123.456920]  __arm64_sys_write+0x20/0x30
[  123.456925]  invoke_syscall+0x4c/0x110
[  123.456930]  el0_svc_common+0x88/0x110
[  123.456935]  do_el0_svc+0x24/0x80
[  123.456940]  el0_svc+0x30/0x80
[  123.456945]  el0t_64_sync+0x84/0x88
```

### 阅读方法

**从下往上读**（从入口到崩溃点）：

1. `el0t_64_sync` — ARM64 异常向量入口（用户态 → 内核态）
2. `el0_svc` — SVC（系统调用）处理
3. `do_el0_svc` → `el0_svc_common` → `invoke_syscall` — 系统调用分发
4. `__arm64_sys_write` — write() 系统调用入口
5. `ksys_write` → `vfs_write` — VFS 写路径
6. `my_driver_write` — **崩溃点**（驱动 write 回调）

### 分析流程

```
步骤 1: 确定崩溃函数 → my_driver_write+0x3c (Call Trace 顶部)
步骤 2: 用 addr2line 定位源码行
步骤 3: 确定调用路径: write syscall → vfs_write → my_driver_write
步骤 4: 检查寄存器值 → x0 = 0 (NULL pointer dereference)
步骤 5: 检查模块标记 → [my_module] 表示该函数来自内核模块
```

### 不完整的 Call Trace

| 现象 | 原因 | 解决方案 |
|------|------|---------|
| `(null)` 或无意义地址 | 栈被破坏（缓冲区溢出覆盖返回地址） | 用 KASAN 检测原始越界操作 |
| 只有几行就断了 | 帧指针未启用（CONFIG_FRAME_POINTER=n） | 启用 CONFIG_FRAME_POINTER=y |
| `?` 前缀的帧 | 栈上的值碰巧像返回地址但不可靠 | 忽略带 `?` 的帧 |
| `0x0` 地址 | 函数指针未初始化被调用 | 检查函数指针初始化 |

### 帧指针与编译优化

```bash
# 确保帧指针启用（内核编译时）
CONFIG_FRAME_POINTER=y
CONFIG_UNWINDER_FRAME_POINTER=y

# 或使用 ORC unwinder（x86，更可靠）
CONFIG_UNWINDER_ORC=y
```

### Call Trace 中的特殊标记

| 标记 | 含义 | 可靠性 |
|------|------|--------|
| 无标记 | 通过帧指针验证 | 可靠 |
| `?` 前缀 | 栈上值看起来像返回地址 | 不可靠 |
| `<0>` | 无效地址 | 忽略 |
| `[my_module]` | 来自内核模块 | 需要用 .ko 解析 |

### HFT 关联应用

在 HFT 内核模块中，Call Trace 帮助定位延迟来源和崩溃原因：

```bash
# 分析交易线程的调用路径
# Call Trace: my_trade_handler → kernel_sendmsg → tcp_sendmsg
# 说明交易数据通过 TCP 发送，可优化网络栈

# 分析崩溃原因
# Call Trace: my_strategy_update → spin_lock → schedule
# 说明在 spinlock 中调用了 schedule → scheduling while atomic
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** Call Trace 从上到下还是从下往上读？

> 从下往上读。最底部是入口点（如 `el0_svc` = 系统调用入口），最顶部是崩溃点（如 `my_driver_write`）。但某些配置下顺序可能不同，看 `pc` 值确认崩溃函数。

**Q2:** Call Trace 出现 `(null)` 或无意义地址是什么原因？

> 栈被破坏。常见原因：(1) 缓冲区溢出覆盖了栈上的返回地址；(2) use-after-free；(3) 栈溢出（递归过深）。需要用 KASAN 或 SLUB debug 检测原始的越界操作。

**Q3:** Call Trace 中 `?` 前缀的帧是什么意思？

> `?` 标记的帧是不可靠的——栈上的值碰巧看起来像返回地址，但可能不是真正的调用链。unwinder 标记这些帧让开发者知道哪些是可信的。

**Q4:** 为什么启用了 CONFIG_FRAME_POINTER 后栈回溯更可靠？

> 帧指针在每个函数的栈帧中保存了上一个帧指针和返回地址。unwinder 通过帧指针链逐帧回溯，结果可靠。没有帧指针时，unwinder 只能猜测。

**Q5:** 内联函数在 Call Trace 中如何表现？

> 内联函数没有独立的栈帧，不会在 Call Trace 中单独出现。addr2line 可能报告内联展开的位置（通过 DWARF 信息），但 Call Trace 只显示调用栈层级。

</details>

## 交叉引用

- [05.6 ch07 寄存器转储解读](../../chapter-07-oops/notes/02-register-dump.md)
- [05.6 ch07 addr2line](../../chapter-07-oops/notes/04-addr2line.md)
- [05.6 ch07 objdump 反汇编](../../chapter-07-oops/notes/05-objdump-disassembly.md)
