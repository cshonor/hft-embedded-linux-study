## 6. 参数验证与内核封装例程

> 用户指针不可信 — **粗检 + 缺页 + 异常表** 三层防御

---

### 一、粗略检查：`access_ok()`

当 syscall 参数是 **用户态指针** 时，内核先做 **线性地址** 检查：

| 检查 | 目的 |
|------|------|
| 地址 **< `PAGE_OFFSET`**（3GB） | 禁止用户指针指向 **内核线性地址空间** |
| 长度不溢出 | 避免 `addr+len` 绕回内核区 |

**局限：** 只保证「不像内核地址」，**不保证** 该地址已映射、可访问。

→ 地址空间布局：[Ch 2](../../chapter-02-memory-addressing/)

---

### 二、细粒度检查：缺页 + 异常表

**惰性策略：** 内核 **直接访问** 用户指针；若无效 → **缺页异常**。

**问题：** 内核态缺页若按普通 kernel fault 处理 → **Kernel Oops**。

**解决：异常表 (Exception Tables)**

| 机制 | 说明 |
|------|------|
| **编译期** | 访问用户内存的指令登记到 **异常表** |
| **缺页时** | `search_exception_tables(fault_ip)` 查表 |
| **命中** | 将 **`eip` 重定向** 到 **fixup 代码**（`.fixup` 段） |
| **fixup** | 安全终止拷贝/读，向 syscall 返回 **`-EFAULT`** |

→ 缺页框架：[Ch 9 section-4](../../chapter-09-process-address-space/notes/section-4-缺页异常.md)

> **深潜可选：** 异常表链接（`__ex_table`）、`fixup` 与 `copy_from_user` 的配合 — 见 `arch/x86/mm/extable.c`。

---

### 三、内核态调用 syscall：`_syscall0` … `_syscall6`

内核线程有时也需触发 syscall 路径。提供 **`_syscallN`** 宏（N = 参数个数）：

- 内联汇编把参数放入寄存器  
- 触发 **`int $0x80`**  
- 在内核中复用同一套 `sys_*()` 逻辑  

---

### 四、本章小结

```
libc API
    ↓ wrapper
syscall nr + 6 regs
    ↓ int 0x80 / sysenter
system_call → sys_call_table → sys_xyz()
    ↓ access_ok + copy_from_user（异常表保护）
具体子系统（进程/内存/调度…）
    ↓
exit_work（调度/信号）→ 返回用户态
```

---

### 五、后续章节索引

| Ch 10 主题 | 继续读 |
|------------|--------|
| 返回路径信号 | [Ch 11 信号](../chapter-11-signals/) 🟡 |
| IDT / iret | [Ch 4 中断与异常](../chapter-04-interrupts-and-exceptions/) 🔴 |
| fork/brk/mmap 实现 | [Ch 3](../chapter-03-processes/) · [Ch 9](../chapter-09-process-address-space/) 🔴 |
| 用户态编程 | [08 TLPI](../../../08-The-Linux-Programming-Interface/) |
| LKD 对照 | Linux Kernel Development Ch 5 |
| vDSO / 无 syscall 计时 | [15 HFT 工程](../../../15-HFT-Low-Latency-Practice/) · modern kernel `vdso(7)` |

---

← [5. 参数传递](./section-5-参数传递.md) · 下一章 [Ch 11 信号](../chapter-11-signals/)
