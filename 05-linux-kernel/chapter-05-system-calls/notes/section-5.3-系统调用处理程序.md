## ③ 系统调用处理程序 · Handler & Parameters

重点：**陷入内核之后，第一步执行的通用入口函数**。  
用户态 **不能直接执行内核代码** — 必须 **陷入（trap）** 切到内核态。

---

### x86_64 `syscall` 路径（概念）

1. 用户执行 **`syscall`** 指令  
   CPU 自动切换特权级，跳转到内核预先设置好的入口 **`entry_SYSCALL_64`**（汇编）
2. 汇编入口完成准备工作：
   - **保存用户态寄存器现场**（保证系统调用结束后能恢复）
   - **切换内核栈**（关键：每个进程拥有独立内核栈）
3. 调用通用处理函数 **`do_syscall_64`**
4. `do_syscall_64`：
   - 读取 `rax` 里的 **系统调用号**，做合法性校验（不能超过表最大下标）
   - 查表调用真正的 `sys_*`
5. 内核函数执行完毕，原路返回；汇编恢复寄存器，**`sysret`** 回到用户态

```c
/* 概念示意 — 非逐字源码 */
nr = regs->ax;
if (nr >= NR_syscalls)
    return -ENOSYS;
sys_call_table[nr](regs);  // 调用真正的系统调用函数
```

流程总览：

```
用户态 syscall 包装
    │
    ▼
syscall 指令
    │
    ▼
entry_SYSCALL_64（保存现场 · 切内核栈）
    │
    ▼
do_syscall_64 ──► sys_call_table[nr] ──► sys_*(...)
    │
    ▼
sysret ──► 回到用户态
```

---

### 关键概念：进程内核栈

| 栈 | 何时用 |
|----|--------|
| **用户栈** | 用户态运行 |
| **进程专属内核栈** | 一旦进入系统调用，立刻切换 |

系统调用里的局部变量、函数调用栈，全部存在 **内核栈**。

---

### 串联：`open()` 进内核后发生什么

当你调用 `open()` 系统调用：  
`sys_openat`（等）会分配 **`struct file`**，在进程 fd 表分配空闲 fd，建立映射。  
细节见 [§3.8](../../chapter-03-process-management/notes/section-3.8-身份PID与资源FD.md) 与本章 README「串联 open」。

---

### 触发方式（x86 演进）

| 机制 | 说明 |
|------|------|
| **`int $0x80`** | 经典 **软件中断**（32 位时代常见） |
| **`sysenter` / `syscall`** | 更快路径（现代 64 位主流为 **`syscall`**） |

#### x86（书中 32 位约定）

| 寄存器 | 用途 |
|--------|------|
| **`eax`** | **系统调用号** 入 · **返回值** 出 |
| **`ebx, ecx, edx, esi, edi, ebp`** | 参数（按序） |

> **x86-64**：号在 **`rax`**，参数常用 **`rdi, rsi, rdx, r10, r8, r9`** — 思想相同：**寄存器传号与参**。

→ 教学对照：[01 Day 20 INT 0x40 API](../../../../projects/P9-os-from-scratch/thirty-days-os/day-20-api/) · 下一节 [§5.4 参数验证](./section-5.4-实现与参数验证.md)


> ↔ [ULK Ch10 §3 分派表与服务例程](../../../18-linux-kernel-deep/chapter-10-system-calls/notes/section-3-分派表与服务例程.md)


<details>
<summary>自测题（点击展开）</summary>

**Q1.** syscall 指令和 int 0x80 的区别？为什么现代 x86 用 syscall？

<details><summary>答案</summary>

int 0x80 是软件中断，需要查 IDT → 中断门 → 权限检查，开销约 200-400ns。syscall 是专门为快速系统调用设计的指令：不查 IDT、直接跳到 MSRs 指定的入口（LSTAR），开销约 50-100ns。现代内核默认用 syscall，int 0x80 仅保留兼容。

</details>

**Q2.** 系统调用处理程序为什么要检查 user_mode？

<details><summary>答案</summary>

内核需要验证请求来自用户态（而非内核态直接调用），防止内核代码绕过安全检查。`access_ok()` 验证用户态指针不会访问内核地址。如果内核代码能直接调 sys_read 传入内核指针，就绕过了所有安全检查。这是 Linux 安全模型的基础。

</details>

</details>
---
