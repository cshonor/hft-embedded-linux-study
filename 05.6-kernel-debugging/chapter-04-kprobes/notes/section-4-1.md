# 4.1 Kprobes 原理与架构

> 🔴 精读 · Part 2: Instrumentation & Memory Debugging

## 本节要点

### Kprobes 是什么

Kprobes 允许在**任意内核函数的任意指令地址**动态插入探针，捕获寄存器和参数，无需重编译内核。

### 三种探针类型

| 类型 | 插入点 | 能力 |
|------|--------|------|
| **kprobe** | 函数入口 | 捕获入口寄存器/参数 |
| **kretprobe** | 函数返回 | 捕获返回值 |
| **jprobe** | 函数入口 | 已废弃 (6.x 移除) |

### 工作原理

```
原始指令:    [函数入口指令]
               ↓
kprobe 注册后:
  → 保存原始指令
  → 将入口指令替换为断点指令 (BRK on ARM64 / INT3 on x86)
  → 执行到断点 → 触发异常 → kprobe 异常处理器
  → 调用 pre_handler(寄存器上下文)
  → 单步执行原始指令
  → 调用 post_handler(寄存器上下文)
  → 恢复执行
```

### 关键数据结构

```c
struct kprobe {
    kprobe_opcode_t *addr;        // 探针地址
    const char *symbol_name;       // 函数名 (可选)
    unsigned int offset;           // 函数内偏移
    kprobe_pre_handler_t pre_handler;   // 入口回调
    kprobe_post_handler_t post_handler;  // 单步后回调
    // ...
};

struct kretprobe {
    struct kprobe kp;              // 内部 kprobe
    kretprobe_handler_t handler;   // 返回回调
    int maxactive;                 // 最大并发实例
    // ...
};
```

### HFT 关联

Kprobes 是 HFT 延迟溯源的核心——在生产环境动态测量内核函数耗时，无需重编译、无需重启。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** kprobe 是如何在不修改源码的情况下插入探针的？

> kprobe 在注册时将目标地址的原始指令保存，替换为架构特定的断点指令（ARM64: BRK，x86: INT3）。CPU 执行到该地址时触发异常，kprobe 的异常处理器保存寄存器上下文并调用 pre_handler。然后单步执行原始指令，调用 post_handler，最后恢复执行。

**Q2:** jprobe 为什么在 6.x 中被移除？

> jprobe 通过修改寄存器来"伪装"成被探测函数，实现复杂且容易出错，在 ARM64 等架构上难以正确实现。6.x 移除 jprobe，推荐用 kprobe + 手动提取参数替代，或直接用 eBPF/bpftrace。


**Q:** kprobes 在 ARM64 上如何实现断点？

> ARM64 使用 BRK 指令（breakpoint）替代 x86 的 INT3。kprobe 在目标地址替换为 BRK #0x4，触发异常进入 kprobe handler。原始指令保存在单独页面（out-of-line execution）。ARM64 还需要 flush icache（指令缓存一致性）。

**Q:** kprobes 为什么不能在某些函数上设置探针？

> 内核标记 `__kprobes` 的函数本身是 kprobe 基础设施，在其上设探针会递归。另外 NMI/硬中断上下文中的某些路径（如 entry code）不允许 kprobe，因为 kprobe 处理需要进程上下文。用 `/sys/kernel/debug/kprobes/blacklist` 查看禁止列表。

</details>

## 交叉引用

- [05.6 ch04 kretprobe](chapter-04-kprobes/notes/section-4-3.md)
