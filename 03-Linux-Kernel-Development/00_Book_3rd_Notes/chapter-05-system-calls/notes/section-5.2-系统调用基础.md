## ② 系统调用基础 · Numbers & Naming

#### 内核侧命名与 ABI

| 约定 | 说明 |
|------|------|
| **`asmlinkage`** | 参数 **仅从栈** 取（历史 ABI 约定） |
| **`sys_` 前缀** | 用户 `bar()` → 内核 **`sys_bar()`** |

#### 系统调用号

| 规则 | 说明 |
|------|------|
| 每个 syscall **唯一编号** | 如 x86 `__NR_read` |
| **`sys_call_table[]`** | 内核维护的 **函数指针表** — 下标 = 号 |
| **号一旦分配永不回收** | 保证 **ABI 稳定** |
| 历史 syscall 被移除 | 槽位填 **`sys_ni_syscall()`** — 只返回 **`-ENOSYS`** |

```c
/* 概念示意 */
asmlinkage long sys_read(unsigned int fd, char __user *buf, size_t count);
```

→ 用户态查号：`unistd.h` / `asm/unistd.h` · `strace` 可见实际号

---
