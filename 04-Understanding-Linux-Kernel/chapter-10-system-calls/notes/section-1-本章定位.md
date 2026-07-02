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

---

← [Ch 10 导读](../README.md) · 下一节 [2. POSIX API](./section-2-POSIX-API与系统调用.md)
