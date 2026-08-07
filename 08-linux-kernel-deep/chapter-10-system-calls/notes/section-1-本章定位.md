## 1. 本章定位

> **ULK Ch 10 System Calls** · 用户态进程如何 **合法** 请求内核服务

---

### 一、本章讲什么

系统调用是应用程序进入内核的 **唯一合法入口**。本章覆盖：

| 主题 | 要点 |
|------|------|
| **API vs syscall** | `libc` 封装、`errno` 约定 |
| **分派** | `sys_call_table`、`sys_xyz()` |
| **陷入/返回** | `int $0x80` vs **`sysenter`** |
| **传参** | 寄存器、最多 6 个参数 |
| **安全** | `access_ok`、缺页 + **异常表** |

Ch 3 fork/exit、Ch 7 sched_*、Ch 9 brk/mmap 的 **用户态入口** 都在本章。

---

### 二、小节导航

| 节 | 主题 |
|----|------|
| [2](./section-2-POSIX-API与系统调用.md) | POSIX API、封装例程、`errno` |
| [3](./section-3-分派表与服务例程.md) | `sys_xyz()`、`sys_call_table` |
| [4](./section-4-进入与退出.md) | `int 0x80`、`sysenter`/`sysexit`、vsyscall |
| [5](./section-5-参数传递.md) | 寄存器传参、`SAVE_ALL` |
| [6](./section-6-参数验证与内核封装.md) | `access_ok`、异常表、`_syscallN` |

---

### 三、在 Linux 链上的位置

```
Ch 4  异常 / IDT / iret 返回
Ch 9  brk / mmap 内核例程
Ch 10 系统调用（本章）— 用户态 ↔ 内核桥梁
Ch 11 信号（syscall 返回路径检查 TIF_SIGPENDING）
08 TLPI  用户态 API 与 syscall 用法
```

HFT：**syscall 开销、vDSO 绕过 syscall**（modern，ULK 2.6 讲 sysenter 前身）是延迟敏感路径的关注点。

### 常见陷阱

1. 把 ULK 讲的 `int $0x80` 当现代 syscall 入口——x86-64 用 `syscall` 指令，从 MSR_LSTAR 加载入口
2. 以为 syscall 号全局唯一——syscall 号是 per-architecture 的，x86-64 和 ARM64 不同
3. 混淆 syscall 和 libc 函数——`printf` 不是 syscall，`write` 才是；`malloc` 不是 syscall，`brk`/`mmap` 才是

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** ULK Ch10 讲的 syscall 入口在现代 x86-64 上有什么变化？

<details><summary>答案</summary>

① `int $0x80` 软中断被 `syscall` 指令取代（快 3-5 倍，不走 IDT 查表）。② 入口地址从 `MSR_LSTAR` 加载（`entry_SYSCALL_64`）。③ CS/SS 从 `MSR_STAR` 加载，不走 GDT 查表。④ 参数传递从栈改为寄存器（`rdi, rsi, rdx, r10, r8, r9`）。⑤ `sysret` 指令快速返回。⑥ vDSO/vvar 页让部分 syscall（`gettimeofday`/`clock_gettime`）完全在用户态完成。

</details>

**Q2.** vDSO（Virtual Dynamic Shared Object）是什么？为什么对 HFT 重要？

<details><summary>答案</summary>

vDSO 是内核映射到每个进程的共享库（`[vdso]` VMA），包含 `gettimeofday`/`clock_gettime`/`getcpu` 等函数。这些函数直接读内核映射的 `vvar` 页（内核定时更新），**不触发 syscall**。开销：~20ns（vs syscall 的 ~100-200ns）。HFT 必须用 vDSO 版的 `clock_gettime(CLOCK_MONOTONIC)`，避免 syscall 开销。`ldd` / `getauxval(AT_SYSINFO_EHDR)` 确认 vDSO 可用。

</details>

**Q3.** 如何减少 HFT 中的系统调用数量？

<details><summary>答案</summary>

① 批量化：`io_uring` 替代多次 `read`/`write`（一次 submit 批量 I/O）。② vDSO：`clock_gettime` 走 vDSO 不进内核。③ 预分配：`mlockall` + 内存池避免 `brk`/`mmap`。④ 轮询 vs 中断：DPDK 用户态轮询替代 `epoll_wait`。⑤ `seccomp` 过滤非法 syscall。测量：`strace -c -p [pid]` 统计 syscall 频率。

</details>

</details>

---

← [Ch 10 导读](../README.md) · 下一节 [2. POSIX API](./section-2-POSIX-API与系统调用.md)
