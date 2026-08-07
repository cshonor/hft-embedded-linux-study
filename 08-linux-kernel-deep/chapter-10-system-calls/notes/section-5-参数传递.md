## 5. 参数传递 (Parameter Passing)

> 不走用户栈拷贝 — **寄存器** 传参，进内核后再 **压栈**

---

### 一、为何不用用户栈

系统调用走 **异常入口**，与用户态普通函数调用路径不同。为：

- 统一异常处理框架  
- 避免直接从用户栈拷贝参数的安全/性能问题  

Linux x86 约定：**寄存器传参**。

---

### 二、寄存器分配（32 位 x86）

| 寄存器 | 用途 |
|--------|------|
| **`eax`** | 系统调用号 |
| **`ebx`** | 第 1 个参数 |
| **`ecx`** | 第 2 个参数 |
| **`edx`** | 第 3 个参数 |
| **`esi`** | 第 4 个参数 |
| **`edi`** | 第 5 个参数 |
| **`ebp`** | 第 6 个参数 |

**最多 6 个参数** — 更多参数需通过 **指针** 间接传递（如 `struct iovec` 数组）。

> **64 位对照：** `syscall` 指令用 `rdi, rsi, rdx, r10, r8, r9`；约定不同，概念相同。

---

### 三、`SAVE_ALL` 宏

进入内核后，**`SAVE_ALL`**（或等价代码）把寄存器 **压入内核栈**，构造类似 C 函数调用的栈帧。

效果：`sys_xyz()` 可以像普通内核 C 函数一样，从栈上读取参数 — 汇编入口与 C 实现解耦。

```
用户态: eax=nr, ebx..ebp=args
    ↓ int 0x80 / sysenter
内核: SAVE_ALL → 内核栈上有 pt_regs
    ↓
sys_call_table[nr](regs) 或从栈取参
```

### 常见陷阱

1. 把 ULK 的参数传递（寄存器 + 栈）当现代版——x86-64 syscall 前 6 参数在 `rdi/rsi/rdx/r10/r8/r9`，无栈传参
2. 混淆 `r10` 和 `rcx`——syscall 用 `r10` 传第 4 参数（因为 `rcx` 被用来存返回地址）
3. 以为可以传任意多参数——Linux syscall 最多 6 个参数，超过的需要用结构体指针

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** x86-64 syscall 的参数传递约定和函数调用约定有什么区别？

<details><summary>答案</summary>

函数调用（System V ABI）：参数在 `rdi/rsi/rdx/rcx/r8/r9`。syscall：参数在 `rdi/rsi/rdx/r10/r8/r9`——第 4 参数从 `rcx` 改为 `r10`，因为 `syscall` 指令用 `rcx` 保存返回地址。这就是为什么汇编 syscall 代码中常见 `mov %rcx, %r10`。超过 6 个参数的 syscall（如 `mmap` 有 6 个、`clone` 有 5 个）正好用满寄存器。

</details>

**Q2.** 用户态指针参数怎么安全传递到内核？

<details><summary>答案</summary>

不能直接解引用！① `access_ok(addr, size)`：检查地址在用户空间范围（防内核地址伪造）。② `copy_from_user(kbuf, ubuf, size)`：安全复制，如果 `ubuf` 无效则返回未复制的字节数。③ `get_user(x, ptr)`/`put_user(x, ptr)`：读/写简单类型（int/long/pointer）。④ `strncpy_from_user()`：安全复制字符串。这些函数都有 page fault fixup——用户指针触发 fault 时返回 `-EFAULT` 而非 panic。

</details>

**Q3.** HFT 如何避免 syscall 参数传递的开销？

<details><summary>答案</summary>

① 共享内存 + 原子操作：数据通过 `mmap(MAP_SHARED)` 共享，用 `std::atomic` 同步，不需要 syscall 传参。② `io_uring` SQE：一次 `mmap` 映射 SQ/CQ ring，之后 submit I/O 只需写 ring + `io_uring_enter`（或甚至不调——SQPOLL 模式）。③ `vDSO`：`clock_gettime` 等直接读共享页，无参数传递开销。④ `seccomp-bpf` 缓存：同一 syscall 反复调用时跳过 seccomp 检查。

</details>

</details>

---

← [4. 进入与退出](./section-4-进入与退出.md) · 下一节 [6. 参数验证](./section-6-参数验证与内核封装.md)
