## 3. 系统调用处理程序与服务例程

---

### 一、命名约定

| 用户可见 | 内核服务例程 |
|----------|--------------|
| `xyz()` syscall | **`sys_xyz()`** |

例：`read()` → `sys_read()`，`brk()` → `sys_brk()`。

---

### 二、系统调用分派表 `sys_call_table`

内核用 **函数指针数组** 将 **系统调用号** 映射到服务例程：

| 要素 | 2.6 典型值 |
|------|------------|
| 表名 | **`sys_call_table[]`** |
| 容量 | **`NR_syscalls`**（如 289） |
| 调用号寄存器 | **`eax`**（x86） |

**分派逻辑（概念）：**

```
nr = eax
handler = sys_call_table[nr]   // 2.6: nr * 4 + 表基址
ret = handler(...)
```

非法调用号 → 返回错误。

---

### 三、与前后章的衔接

| syscall | 内核实现章节 |
|---------|--------------|
| `fork` / `exit` / `wait` | [Ch 3](../../chapter-03-processes/notes/section-6-创建与销毁.md) |
| `brk` / `mmap` | [Ch 9](../../chapter-09-process-address-space/) |
| `nice` / `sched_setscheduler` | [Ch 7](../../chapter-07-process-scheduling/notes/section-6-调度相关系统调用.md) |
| `gettimeofday` 等 | [Ch 6](../../chapter-06-timing/notes/section-6-定时相关系统调用.md) |

本章讲 **如何到达** `sys_*()`，各章讲 **里面做什么**。

### 常见陷阱

1. 把 ULK 的 `sys_call_table` 当唯一分派方式——x86-64 仍用 `sys_call_table` 但入口不同，且加了 spectre 缓解
2. 以为 syscall 号和 ULK 时代一样——6.x 新增了大量 syscall（io_uring、pidfd、clone3 等），编号变了
3. 混淆 `sys_xxx` 和 `SYSCALL_DEFINE1/2/...`——现代内核用 `SYSCALL_DEFINEn` 宏定义 syscall，不是直接 `sys_xxx`

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 现代内核如何定义和注册一个新 syscall？

<details><summary>答案</summary>

```c
SYSCALL_DEFINE3(my_syscall, int, arg1, char __user *, arg2,
                size_t, arg3)
{
    // 参数验证
    if (arg1 < 0) return -EINVAL;
    char *kbuf = kmalloc(arg3, GFP_KERNEL);
    if (!kbuf) return -ENOMEM;
    if (copy_from_user(kbuf, arg2, arg3)) {
        kfree(kbuf); return -EFAULT;
    }
    // ... 处理 ...
    kfree(kbuf);
    return 0;
}
```
`SYSCALL_DEFINEn` 宏展开后生成 `sys_my_syscall`，自动加入 `sys_call_table`。n = 参数个数。

</details>

**Q2.** `SYSCALL_DEFINEn` 宏相比直接 `asmlinkage long sys_xxx()` 有什么优势？

<details><summary>答案</summary>

① 类型安全：宏对每个参数做类型检查。② 防溢出攻击：宏将函数名和参数签名组合成唯一符号，增加攻击者预测难度。③ `__SYSCALL_DEFINEx` 内部加 `asmlinkage` + `__visible` + spectre 缓解（`__x86_indirect_thunk`）。④ 自动生成 syscall 表条目。ULK 时代的直接 `asmlinkage long sys_xxx()` 已废弃。

</details>

**Q3.** 如何查找某个 syscall 号？

<details><summary>答案</summary>

① `ausyscall --dump`（audit 包）列出所有 syscall 号。② `/usr/include/asm/unistd_64.h`（x86-64 syscall 号）。③ `man 2 syscall`。④ `/proc/sys/kernel/last_pid` 不是 syscall 号。⑤ `strace -e trace=read cat /dev/null` 查看实际调用。注意：x86-64、ARM64、x86-32 的 syscall 号不同，跨架构不能硬编码。

</details>

</details>

---

← [2. POSIX API](./section-2-POSIX-API与系统调用.md) · 下一节 [4. 进入与退出](./section-4-进入与退出.md)
> ↔ [LKD Ch05 §5.3 系统调用处理程序](../../../05-linux-kernel/chapter-05-system-calls/notes/section-5.3-系统调用处理程序.md)
