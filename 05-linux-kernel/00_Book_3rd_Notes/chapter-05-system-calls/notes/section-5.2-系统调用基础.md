## ② 系统调用基础 · Numbers & Naming

---

### 1. 系统调用号

每一个内核提供的服务，分配一个独一无二的 **数字编号（系统调用号）**。

示例（x86_64，号随内核/ABI 以头文件为准）：

| 符号 | 常见号（示意） |
|------|----------------|
| `__NR_read` | 0 |
| `__NR_write` | 1 |
| `__NR_open` | 2 |

内核内部维护一张大表：**`sys_call_table`（系统调用表）**

| | |
|--|--|
| 数组下标 | = 系统调用号 |
| 数组内容 | = 对应内核处理函数指针 `sys_xxx()` |

| 规则 | 说明 |
|------|------|
| 每个 syscall **唯一编号** | 如 x86 `__NR_read` |
| **号一旦分配永不回收** | 保证 **ABI 稳定** |
| 历史 syscall 被移除 | 槽位填 **`sys_ni_syscall()`** — 只返回 **`-ENOSYS`** |

---

### 2. 调用流程（极简版）

1. 用户程序把 **系统调用号、调用参数** 放入 CPU 指定寄存器；
2. 触发 CPU 特权切换指令，陷入内核；
3. 内核取出调用号，查 `sys_call_table`，执行对应内核函数；
4. 执行完成，把返回值放入寄存器，切回用户态继续运行。

---

### 3. 陷入指令（按架构）

| 架构 | 指令 |
|------|------|
| x86_32 | `int 0x80`（软中断） |
| x86_64 | `syscall` / `sysret`（专用快速路径，性能更高） |
| ARM64 | `svc #0` |

---

### 4. 返回值约定

| 内核约定（惯例） | 含义 |
|------------------|------|
| 正数 / 0 | 成功 |
| 负数 | 错误码（内核负错误码） |

libc 会把负错误码转换成全局变量 **`errno`**（用户态看到的是正 errno + 函数返回 -1）。

---

### 内核侧命名与 ABI

| 约定 | 说明 |
|------|------|
| **`asmlinkage`** | 参数 **仅从栈** 取（历史 ABI 约定） |
| **`sys_` 前缀** | 用户 `bar()` → 内核 **`sys_bar()`** |

```c
/* 概念示意 */
asmlinkage long sys_read(unsigned int fd, char __user *buf, size_t count);
```

→ 用户态查号：`unistd.h` / `asm/unistd.h` · `strace` 可见实际号 · 下一节 [§5.3 入口处理](./section-5.3-系统调用处理程序.md)


> ↔ [ULK Ch10 §2 POSIX-API与系统调用](../../../../19-linux-kernel-deep/chapter-10-system-calls/notes/section-2-POSIX-API与系统调用.md)


<details>
<summary>自测题（点击展开）</summary>

**Q1.** 系统调用号存在哪里？为什么每个架构不同？

<details><summary>答案</summary>

syscall 号定义在 `arch/*/include/uapi/asm/unistd.h`。x86_64 的 read=0, write=1, open=2...。不同架构不同是因为历史原因（x86 用 int 0x80，ARM 用 SVC 指令）。用户态 libc 的 read() 内部根据架构填入正确的 syscall 号。这保证了源码可移植但二进制不可移植。

</details>

**Q2.** 为什么现代内核不鼓励新增系统调用？

<details><summary>答案</summary>

新增 syscall 是永久 ABI 承诺：一旦合入 mainline 就不能删除/改语义（会破坏用户态程序）。替代方案：1) io_uring（一个 syscall 搞定任意 IO 操作）；2) eBPF（用户态程序注入内核运行）；3) /proc 或 /sys 接口（不需要新 syscall 号）。

</details>

</details>
---
