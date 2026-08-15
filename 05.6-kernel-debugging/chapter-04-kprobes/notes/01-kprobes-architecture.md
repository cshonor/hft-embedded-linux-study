# 4.1 Kprobes 原理与架构

> 🔴 精读 · Part 2: Instrumentation & Memory Debugging

## 本节要点

Kprobes 允许在**任意内核函数的任意指令地址**动态插入探针，捕获寄存器和参数，无需重编译内核。

## 三种探针类型

| 类型 | 插入点 | 能力 | 状态 |
|------|--------|------|------|
| **kprobe** | 函数入口 | 捕获入口寄存器/参数 | ✅ |
| **kretprobe** | 函数返回 | 捕获返回值 | ✅ |
| **jprobe** | 函数入口 | 直接获取函数参数 | ❌ 6.x 移除 |

## 工作原理

```
原始指令:    [函数入口指令]
               ↓
kprobe 注册后:
  → 保存原始指令
  → 将入口指令替换为断点指令 (BRK on ARM64 / INT3 on x86)
  → 执行到断点 → 触发异常 → kprobe 异常处理器
  → 调用 pre_handler(寄存器上下文)
  → 单步执行原始指令 (out-of-line execution)
  → 调用 post_handler(寄存器上下文)
  → 恢复执行
```

### 断点指令架构差异

| 架构 | 断点指令 | 异常类型 | 额外操作 |
|------|---------|---------|---------|
| x86_64 | INT3 (0xCC) | #BP | 无 |
| ARM64 | BRK #0x4 | Synchronous Abort | flush icache |
| ARM32 | BKPT #0 | Prefetch Abort | flush icache |
| RISC-V | ebreak | Exception | flush icache |

### Out-of-Line Execution

```
kprobe 不在原地址执行原始指令（因为已被替换为断点）
而是将原始指令拷贝到单独的缓冲区执行:

原地址:        [BRK 指令]  ← 触发异常
                      ↓
kprobe 缓冲区: [原始指令]  ← 单步执行
                [返回指令]  ← 跳回原地址+4
```

## 关键数据结构

```c
struct kprobe {
    kprobe_opcode_t *addr;         // 探针地址（运行时解析）
    const char *symbol_name;       // 函数名（可选，自动解析为 addr）
    unsigned int offset;           // 函数内偏移
    kprobe_pre_handler_t pre_handler;   // 入口回调
    kprobe_post_handler_t post_handler;  // 单步后回调
    // ...
};

struct kretprobe {
    struct kprobe kp;              // 内部 kprobe
    kretprobe_handler_t handler;   // 返回回调
    int maxactive;                 // 最大并发实例数
    // ...
};
```

## kprobe 黑名单

```bash
# 某些函数不允许设置 kprobe（会导致递归或崩溃）
cat /sys/kernel/debug/kprobes/blacklist | head -20
# 输出示例:
# 0xffffffff80001000  0xffffffff80001100  kprobe_handler
# 0xffffffff80001200  0xffffffff80001300  entry_SYSCALL_64

# 标记 __kprobes 的函数会自动加入黑名单
# 标记 NOKPROBE_SYMBOL() 的函数也会加入
```

```c
// 在代码中标记函数为 kprobe 禁区
NOKPROBE_SYMBOL(my_critical_function);

// 或使用 __kprobes 修饰符
static int __kprobes my_kprobe_handler(struct kprobe *p, struct pt_regs *regs)
{
    // 这个函数本身不能被 kprobe
    return 0;
}
```

## kprobe 开销分析

| 操作 | 开销 | 说明 |
|------|------|------|
| 断点异常 | ~200-500ns | CPU 触发异常 + 上下文保存 |
| pre_handler 执行 | 取决于代码 | 通常 <1μs |
| 单步执行原始指令 | ~100ns | out-of-line execution |
| post_handler 执行 | 取决于代码 | 通常 <1μs |
| 恢复执行 | ~100ns | 上下文恢复 |
| **总计（空回调）** | ~1-5μs | |

## HFT 关联

Kprobes 是 HFT 延迟溯源的核心——在生产环境动态测量内核函数耗时，无需重编译、无需重启。

```bash
# HFT 典型用法：测量 schedule() 耗时分布
echo 'p:my_in schedule' >> /sys/kernel/tracing/kprobe_events
echo 'r:my_out schedule $retval' >> /sys/kernel/tracing/kprobe_events
echo 'hist:keys=common_pid:vals=$wallclock_ns:sort=vals' > \
    /sys/kernel/tracing/events/kprobes/my_out/trigger
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** kprobe 是如何在不修改源码的情况下插入探针的？

> kprobe 在注册时将目标地址的原始指令保存，替换为架构特定的断点指令（ARM64: BRK，x86: INT3）。CPU 执行到该地址时触发异常，kprobe 的异常处理器保存寄存器上下文并调用 pre_handler。然后单步执行原始指令（out-of-line），调用 post_handler，最后恢复执行。

**Q2:** jprobe 为什么在 6.x 中被移除？

> jprobe 通过修改寄存器来"伪装"成被探测函数，实现复杂且容易出错，在 ARM64 等架构上难以正确实现。6.x 移除 jprobe，推荐用 kprobe + 手动提取参数替代，或直接用 eBPF/bpftrace。

**Q3:** kprobes 在 ARM64 上如何实现断点？

> ARM64 使用 BRK 指令（breakpoint）替代 x86 的 INT3。kprobe 在目标地址替换为 BRK #0x4，触发异常进入 kprobe handler。原始指令保存在单独页面（out-of-line execution）。ARM64 还需要 flush icache（指令缓存一致性）。

**Q4:** kprobes 为什么不能在某些函数上设置探针？

> 内核标记 `__kprobes` 的函数本身是 kprobe 基础设施，在其上设探针会递归。另外 NMI/硬中断上下文中的某些路径（如 entry code）不允许 kprobe，因为 kprobe 处理需要进程上下文。用 `/sys/kernel/debug/kprobes/blacklist` 查看禁止列表。

**Q5:** kprobe 的 out-of-line execution 是什么？为什么需要？

> kprobe 将原始指令拷贝到单独缓冲区执行，而不是在原地址执行。因为原地址的指令已被替换为断点指令，直接在原地址单步会再次触发断点。out-of-line execution 将原始指令拷贝到安全缓冲区，执行后跳回原地址的下一条指令。

</details>

## 交叉引用

- [05.6 ch04 kprobe 入口探针](../../chapter-04-kprobes/notes/02-kprobe-entry-handler.md)
- [05.6 ch04 kretprobe](../../chapter-04-kprobes/notes/03-kretprobe-return-handler.md)
- [05.6 ch09 ftrace](../../chapter-09-ftrace/notes/01-ftrace-architecture-tracefs.md)
