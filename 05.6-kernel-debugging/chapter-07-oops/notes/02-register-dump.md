# 寄存器转储解读

> 🔴 精读

## 概念详解

### Oops 日志中的寄存器信息

Oops 日志的核心是寄存器转储——它记录了崩溃瞬间 CPU 的完整状态。通过分析寄存器值，可以快速判断错误类型和原因。

### ARM64 寄存器一览

| 寄存器 | 用途 | 在 Oops 中的意义 |
|--------|------|-----------------|
| `pc` | 程序计数器 | 崩溃指令地址 |
| `lr` (x30) | 链接寄存器 | 调用者地址（函数返回地址） |
| `sp` | 栈指针 | 当前栈顶 |
| `x29` (fp) | 帧指针 | 当前栈帧基址 |
| `x0-x8` | 参数/返回寄存器 | 函数参数和返回值 |
| `x9-x28` | 临时寄存器 | 编译器自由使用 |
| `pstate` | 处理器状态 | NZCV 标志 + 中断状态 |

### 完整 Oops 日志结构

```
[  123.456789] Internal error: Oops: 96000004 [#1] PREEMPT SMP
[  123.456790] Modules linked in: my_module nfnetlink
[  123.456795] CPU: 2 PID: 1234 Comm: my_app Tainted: G  W  6.1.63 #1
[  123.456800] Hardware name: Raspberry Pi 5 Model B (DT)
[  123.456805] pstate: 80400005 (Nzcv daif +PAN -UAO -TCO -DIT -SSBS BTYPE=--)
[  123.456810] pc : my_driver_write+0x3c/0x100 [my_module]
[  123.456815] lr : vfs_write+0xf4/0x2b0
[  123.456820] sp : ffff80000a3c3d80
[  123.456825] x29: ffff80000a3c3d80 x28: 0000000000000000
...
[  123.456895] x1 : ffff000056789abc x0 : 0000000000000000  ← NULL!
```

### 关键字段详细解读

| 字段 | 含义 | 分析要点 |
|------|------|---------|
| `pc` | 程序计数器 | 崩溃指令地址，用 addr2line 定位源码行 |
| `lr` | 链接寄存器 | 调用者地址，帮助重建调用链 |
| `sp` | 栈指针 | 检查栈是否溢出（接近栈底则溢出） |
| `x0` | 第1个参数 | 常见 NULL 指针解引用的元凶 |
| `pstate` | 处理器状态 | 判断中断是否禁用、条件标志 |
| `[#1]` | Oops 编号 | 第 1 次（多次表示级联） |
| `Tainted` | 污染标志 | 判断内核状态是否纯净 |
| `PREEMPT SMP` | 内核特性 | 抢占式多核内核 |

### pstate 字段详解

```
pstate: 80400005 (Nzcv daif +PAN -UAO -TCO -DIT -SSBS BTYPE=--)
```

| 标志 | 含义 | 值 |
|------|------|-----|
| N | 负标志 | 1 (上次运算结果为负) |
| z | 零标志 | 0 |
| c | 进位标志 | 0 |
| v | 溢出标志 | 0 |
| daif | 中断屏蔽 | 0 (中断未禁用) |
| +PAN | 特权访问禁止 | 启用 |
| -UAO | 用户访问覆盖 | 关闭 |

### Tainted 标志详解

| 字母 | 含义 | 说明 |
|------|------|------|
| G | 非 GPL 模块 | 加载了非 GPL 许可的模块 |
| P | 已应用补丁 | 内核打了非主线补丁 |
| O | 树外模块 | 加载了树外编译的模块 |
| E | 未签名模块 | 模块未签名 |
| W | 之前有警告 | 内核发出过警告 |
| B | 页面已污染 | 之前发生过坏页 |
| C | staging 驱动 | 加载了 staging 驱动 |
| L | 软锁定 | 之前发生过软死锁 |

### 常见寄存器模式与错误对应

| 寄存器值 | 可能的错误类型 |
|---------|--------------|
| `x0 = 0` | NULL 指针解引用（x0 常为第一个参数） |
| `x0 = 0xdead...` | Use-after-free（已释放标记） |
| `sp` 接近栈底 | 栈溢出 |
| `pc` 在模块地址段 | 模块代码崩溃 |
| `pc = 0` | 函数指针未初始化 |
| 多个寄存器为 `0x6b6b6b6b` | SLUB debug 已释放内存模式 |

### HFT 关联应用

在 HFT 内核模块调试中，快速分析寄存器转储可以：

1. **立即定位错误类型**：x0=0 → NULL deref，sp 异常 → 栈溢出
2. **推断调用参数**：x0-x7 是函数参数，帮助理解崩溃时的上下文
3. **判断中断状态**：pstate 的 daif 位判断崩溃时中断是否禁用

```c
// HFT 模块中常见的 NULL deref 模式
static ssize_t my_write(struct file *f, const char __user *buf,
                        size_t len, loff_t *off)
{
    struct my_ctx *ctx = f->private_data;
    // 如果 ctx 未初始化或已释放 → x0(ctx) = 0 → Oops
    return copy_from_user(ctx->buffer, buf, len);  // x0 = ctx = NULL!
}
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** Oops 日志中 `pc : my_driver_write+0x3c/0x100` 的各部分含义？

> `my_driver_write` = 函数名，`+0x3c` = 崩溃点在函数内偏移 0x3c 字节处，`/0x100` = 函数总大小 0x100 字节。用 `addr2line` 或 `objdump -d` 可以定位到具体源码行。

**Q2:** ARM64 的 `pstate: 80400005` 中的标志位代表什么？

> pstate 是处理器状态寄存器。`N=1` (负标志), `z=0`, `c=0`, `v=0` (条件标志), `daif=0` (中断未禁用), `+PAN` (特权访问禁止), `-UAO` (用户访问覆盖关闭)。这些标志帮助判断崩溃时的 CPU 状态（如中断是否禁用）。

**Q3:** 寄存器值为 `0x6b6b6b6b` 意味着什么？

> SLUB debug 在释放内存时将内容填充为 `0x6b`。如果寄存器指向已释放的内存，读取的值会是 `0x6b6b6b6b`，表示 use-after-free。类似地，`0xab` 表示未初始化内存。

**Q4:** Tainted 标志 `G W` 对调试有什么影响？

> `G` 表示加载了非 GPL 模块，`W` 表示之前有警告。社区开发者可能拒绝处理带 Tainted 标志的 Oops 报告。HFT 自定义模块应使用 GPL 许可避免此问题。

**Q5:** 如何判断 Oops 时中断是否被禁用？

> 看 pstate 中的 `daif` 字段。`daif=0` 表示中断未禁用。如果 `i=1`（IRQ 屏蔽），说明 Oops 发生在禁用中断的临界区中。

</details>

## 交叉引用

- [05.6 ch07 Oops vs Panic](../../chapter-07-oops/notes/01-oops-vs-panic.md)
- [05.6 ch07 栈回溯分析](../../chapter-07-oops/notes/03-call-trace-analysis.md)
- [05.6 ch07 addr2line](../../chapter-07-oops/notes/04-addr2line.md)
