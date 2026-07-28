## ⑧ 在栈上的静态分配

内核 **同样用 C 栈** 放局部变量，但栈 **极小、不可增长、溢出即灾难** — 规则比用户态 **硬得多**。

#### 内核栈大小

| 配置 | 典型大小 |
|------|----------|
| **传统 x86** | **2 页 = 8KB**（`THREAD_SIZE`） |
| **4KB 内核栈** | 单页 — **更紧张** |
| **中断栈** | 部分 arch **独立中断栈** — 不与进程内核栈混用（Ch 7） |
| **vmap 栈** |  guard page — **溢出触发 fault** 而非 silent corrupt |

```
每个 task_struct
    └── thread_info / 内核栈（固定 4~8KB）
            ├── 函数调用帧
            ├── 局部变量  ← 必须小
            └── 中断嵌套时同一栈（无 vmap 时）
```

#### 禁止模式

| 错误 | 后果 |
|------|------|
| **`char buf[65536]` 局部数组** | **栈溢出** — 覆盖 **thread_info**、返回地址 |
| **大 `struct` 值拷贝进栈** | 同样危险 |
| **无限递归** | 瞬间耗尽 |
| **`alloca` 大块** | 等价于栈上大数组 |

#### 正确替代

| 需求 | 做法 |
|------|------|
| **几 KB 临时缓冲** | **`kmalloc(..., GFP_KERNEL)`** — 进程上下文 |
| **中断里小缓冲** | **静态 per-CPU 缓冲** 或 **预分配 pool** |
| **固定类型高频** | **`kmem_cache_alloc`** |
| **编译期常量表** | **`static const`** 放 **.rodata** |

```c
/* 坏 */
void bad(void) {
    char tmp[8192];  /* 可能已超过整栈 */
}

/* 好 */
void good(void) {
    char *tmp = kmalloc(8192, GFP_KERNEL);
    if (!tmp) return;
    /* ... */
    kfree(tmp);
}
```

#### 与用户栈对比

| | 用户栈 | 内核栈 |
|--|--------|--------|
| 默认大小 | **~8MB**（可调 `ulimit`） | **8KB 量级** |
| 溢出 | SIGSEGV（常） | **破坏内核** — panic / 难查 |
| 大数组 | 仍不推荐 | **绝对禁止** |

**HFT：** 用户态 **策略栈** 也不放大数组 — **`thread_local` ring + mmap** 放堆/映射区。内核 **NAPI** 处理函数 **栈帧要浅** — 深调用链 + 局部变量 = **隐性 latency**（cache miss + 栈 touch）。

→ [Ch 2 内核栈](../../chapter-02/getting-started-with-the-kernel/) · [Ch 7 中断栈](../../chapter-07-interrupts-and-interrupt-handlers/) · [Ch 12.5 kmalloc](./section-12.5-kmalloc-与-kfree.md)

---
