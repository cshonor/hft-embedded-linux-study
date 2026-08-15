# Oops 与 Panic：内核错误的两种结局

> 🔴 精读 · Part 3: Diagnostics & Advanced Tools

## 概念详解

### Oops 是什么

Oops 是内核遇到非致命错误时打印的诊断信息。它记录了崩溃时的 CPU 状态（寄存器、栈、调用链），然后杀死当前进程，内核继续运行。Oops 不意味着系统立即不可用，但内核状态可能已损坏——继续运行是不安全的。

### Panic 是什么

Panic 是内核遇到致命错误时彻底放弃运行。系统停止所有处理，打印 panic 信息后挂起或重启。Panic 发生在 Oops 无法恢复的场景（如中断上下文中 Oops）或开发者主动调用 `panic()`。

### Oops 触发条件

| 触发条件 | 典型原因 | 错误码示例 |
|---------|---------|-----------|
| NULL 指针解引用 | 指针未初始化/已释放 | `Unable to handle kernel NULL pointer dereference` |
| 非法内存访问 | 访问未映射地址 | `Unable to handle kernel paging request` |
| 非法指令 | 代码损坏/跳转到错误地址 | `Internal error: Oops: 96000004` |
| 断言失败 | `BUG_ON()` / `WARN_ON()` | `kernel BUG at xxx` |
| 栈溢出 | 递归过深/局部变量过大 | `stack-protector: Kernel stack is corrupted` |
| 对齐错误 | ARM64 严格对齐检查 | `Alignment fault` |

### Oops vs Panic 对比

| 特性 | Oops | Panic |
|------|------|-------|
| 触发 | 非致命错误（NULL deref, 越界等） | 致命错误 / `panic()` 主动调用 |
| 系统状态 | 继续运行（杀死当前进程） | 系统完全停止 |
| 后续操作 | 可继续操作（但不安全） | 必须重启 |
| 输出信息 | 寄存器转储 + Call Trace | Oops 信息 + panic 调用栈 |
| 配置控制 | `CONFIG_BUG=y` | `panic_on_oops=1` 可升级 Oops 为 panic |
| 中断上下文 | Oops → 自动升级为 panic | 直接 panic（无法 kill 进程） |

### 控制 Oops 行为

```bash
# Oops 后是否 panic
cat /proc/sys/kernel/panic_on_oops    # 0=继续, 1=panic
echo 1 > /proc/sys/kernel/panic_on_oops  # 生产环境常设 1

# Panic 后自动重启延迟（秒）
cat /proc/sys/kernel/panic    # 0=不重启, >0=延迟秒数
echo 10 > /proc/sys/kernel/panic  # 10 秒后自动重启
```

### Oops 日志的基本结构

```
[  123.456789] Internal error: Oops: 96000004 [#1] PREEMPT SMP
[  123.456790] Modules linked in: my_module nfnetlink
[  123.456795] CPU: 2 PID: 1234 Comm: my_app Tainted: G  W  6.1.63 #1
[  123.456800] Hardware name: Raspberry Pi 5 Model B (DT)
[  123.456805] pstate: 80400005 (Nzcv daif +PAN -UAO)
[  123.456810] pc : my_driver_write+0x3c/0x100 [my_module]
[  123.456815] lr : vfs_write+0xf4/0x2b0
[  123.456820] sp : ffff80000a3c3d80
...
[  123.457000] Code: xxxx xxxx (xxxx) xxxx xxxx
[  123.457005] ---[ end trace 0000000000000000 ]---
```

关键字段：
- `[#1]`：第 1 次 Oops（多次表示级联崩溃）
- `Tainted: G W`：内核污染标志
- `pc`：崩溃指令地址
- `Code:` 行：崩溃地址附近的机器码

### Oops 的 "Code:" 行

`Code:` 行显示崩溃地址附近的机器码字节，崩溃指令用 `(xx)` 标记：

```
Code: a9bf7bfd 910003fd f9000fe0 b9400000 (b9400000) 110007e0
                                                    ^ 崩溃指令
```

用途：
- 在无 DEBUG_INFO 时通过机器码特征推断指令
- 确认反汇编结果是否正确
- 快速判断指令类型（如 `b9` 开头 = ARM64 LDR 指令）

### HFT 关联应用

HFT 生产环境应设 `panic_on_oops=1` + `panic=5`——Oops 后 5 秒自动重启，避免在不确定状态下继续交易。

```bash
# HFT 生产环境推荐配置
echo 1 > /proc/sys/kernel/panic_on_oops
echo 5 > /proc/sys/kernel/panic
# 或写入 /etc/sysctl.d/99-hft.conf
```

在 staging 环境保持 `panic_on_oops=0`，以便收集 Oops 信息后继续调试。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** Oops 后系统继续运行为什么不安全？

> Oops 意味着内核数据结构可能已损坏（如链表断裂、引用计数错误）。继续运行可能导致级联崩溃、数据损坏或安全漏洞。生产环境建议设 `panic_on_oops=1` 让系统重启到已知良好状态。

**Q2:** 什么情况下 Oops 会自动变成 panic？

> 三种情况：(1) `panic_on_oops=1` 内核参数；(2) Oops 发生在中断上下文（无法 kill 进程，必须 panic）；(3) Oops 发生在持有关键锁的临界区。

**Q3:** Oops 中的 "Code:" 行有什么用？

> 显示崩溃地址附近的机器码字节。用于确认反汇编结果是否正确，或在无 DEBUG_INFO 时通过机器码特征推断指令。格式：`Code: xx xx xx xx xx xx ...`，崩溃指令用 `(xx)` 标记。

**Q4:** `[#1]` 和 `[#3]` 在 Oops 日志中代表什么？

> `[#1]` 表示这是第 1 次 Oops。`[#3]` 表示第 3 次——多次 Oops 通常意味着第一次 Oops 损坏了数据结构，导致后续级联崩溃。应该分析第一个 Oops（编号最小的），而非最后一个。

**Q5:** HFT 生产环境为什么要设 `panic=5` 而不是 `panic=0`？

> `panic=5` 表示 panic 后 5 秒自动重启。`panic=0` 表示不重启，系统挂死等待人工干预。HFT 要求快速恢复，5 秒延迟足够保存 panic 信息到 kdump（如果配置了），又能快速恢复服务。

</details>

## 交叉引用

- [05.6 ch07 寄存器转储解读](chapter-07-oops/notes/02-register-dump.md)
- [05.6 ch07 addr2line 定位源码行](chapter-07-oops/notes/04-addr2line.md)
- [05.6 ch10 panic/lockup 检测](chapter-10-panic-lockup/notes/01-panic-causes.md)
