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
| 用户态编程 | [08 TLPI](../../../03-linux-userspace-api/) |
| LKD 对照 | Linux Kernel Development Ch 5 |
| vDSO / 无 syscall 计时 | [16 HFT 工程](../../../17-hft-engineering/) · modern kernel `vdso(7)` |

### 常见陷阱

1. 以为 `access_ok()` 就能保证指针安全——`access_ok()` 只检查地址范围，不保证页已映射或可写
2. 在内核中直接 `memcpy()` 用户指针——必须用 `copy_from_user()`，否则可能 panic 或安全漏洞
3. 忽略 `__user` 标注——`__user` 是 sparse 工具的标注，帮助发现未经验证的用户指针使用

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** `access_ok()` 检查什么？不检查什么？

<details><summary>答案</summary>

检查：地址 + 大小在用户空间范围内（x86-64: `addr + size <= TASK_SIZE`，通常 0x7fffffffffff）。不检查：① 页是否已映射（可能仍触发 #PF）。② 页是否可写（`access_ok(VERIFY_WRITE, ...)` 已废弃，现代只检查范围）。③ 指针是否指向有效数据。`access_ok()` 是第一道防线，`copy_from_user()` 是真正的安全网（有 fault fixup）。

</details>

**Q2.** `copy_from_user()` 为什么比 `memcpy()` 安全？

<details><summary>答案</summary>

① `access_ok()` 预检查。② 每次访问都注册在异常表中（`__ex_table`）。③ 如果用户页不可读（未映射/swap/权限不对），触发 #PF → `fixup_exception()` 找到 fixup 地址 → 跳到 `copy_from_user` 的错误返回点 → 返回未复制的字节数。`memcpy()` 直接解引用用户指针 → 如果页不可用 → 内核态 #PF → `die()`/panic。这就是为什么内核必须用 `copy_from_user`。

</details>

**Q3.** HFT 中如何减少 `copy_from_user`/`copy_to_user` 的开销？

<details><summary>答案</summary>

① 共享内存：`mmap(MAP_SHARED)` 让用户态和内核态共享物理页，零拷贝。② `io_uring`：SQE/CQE 通过共享 ring 传递，`io_uring_enter` 只通知不拷贝。③ `splice`/`sendfile`：内核内数据搬运，不经过用户空间。④ `MSG_ZEROCOPY`：网络发送零拷贝（网卡 DMA 直接读用户页）。⑤ 大块数据：一次 `copy_from_user` 大块 > 多次小块（减少 `access_ok` 调用次数）。

</details>

</details>

---

← [5. 参数传递](./section-5-参数传递.md) · 下一章 [Ch 11 信号](../chapter-11-signals/)
> ↔ [LKD Ch05 §5.4 实现与参数验证](../../../05-linux-kernel/00_Book_3rd_Notes/chapter-05-system-calls/notes/section-5.4-实现与参数验证.md)
