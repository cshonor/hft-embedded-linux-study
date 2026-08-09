# Ch 10 系统调用 · System Calls

> **Understanding the Linux Kernel** 3rd · Bovet & Cesati · **🔴 HFT 精读**  
> 用户态 → 内核的唯一合法入口 — 分派、传参、验证

---

## ⚠️ 过时标记（ULK3 基于 Linux 2.6，现为 6.x）

| ULK3 讲的 | 现代变化 | 替代资料 |
|-----------|---------|----------|
| `sys_call_table` | x86-64 仍用，但入口改用 `syscall` 指令 | [System call table for x86-64](https://blog.rchapman.org/posts/Linux_System_Call_Table_for_x86_64/) |
| **`0x80` 软中断入口** | **已废弃**，改用 `syscall` 指令 | [vDSO and system calls](https://lwn.net/Articles/627232/) |
| 参数验证 | 概念类似，但 helper 函数更新 | [Kernel doc: syscall API](https://docs.kernel.org/core-api/syscalls.html) |
| `sys_*` 命名 | 现代 `SYSCALL_DEFINE*` 宏 | [Kernel doc: syscall wrappers](https://docs.kernel.org/core-api/syscalls.html) |

> **原则**：系统调用概念框架不变（用户态→内核态切换、参数传递、验证），但入口机制和命名约定已变。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. POSIX API 与系统调用 | [notes/section-2-POSIX-API与系统调用.md](./notes/section-2-POSIX-API与系统调用.md) |
| 3. 分派表与服务例程 | [notes/section-3-分派表与服务例程.md](./notes/section-3-分派表与服务例程.md) |
| 4. 进入与退出 | [notes/section-4-进入与退出.md](./notes/section-4-进入与退出.md) |
| 5. 参数传递 | [notes/section-5-参数传递.md](./notes/section-5-参数传递.md) |
| 6. 参数验证与内核封装 | [notes/section-6-参数验证与内核封装.md](./notes/section-6-参数验证与内核封装.md) |

---

## 相关

- 上一章：[chapter-09-process-address-space/](../chapter-09-process-address-space/)
- 下一章：[chapter-11-signals/](../chapter-11-signals/)
- 衔接：[Ch 4 异常返回](../chapter-04-interrupts-and-exceptions/notes/section-8-中断返回.md) · [08 TLPI](../../03-linux-userspace-api/)
- [OUTLINE.md](../OUTLINE.md) · [LEARNING_PLAN.md](../LEARNING_PLAN.md)
