# Ch 20 · 系统调用

> **原书第 20 章** · HFT **🔴** · 官方源码标签 `osbook_day20`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **用户态隔离：** **Ring 3** · **TSS.RSP0** · **`syscall`/`sysret`** · **`syscall_table`**

---

### 本章结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 隔离** | **Ring 0/3** · 页表 **User** | 应用 **碰不到内核** |
| **② 栈** | **TSS** · **RSP0** | Ring3 中断 **安全回内核** |
| **③ 调试** | **#PF/#GP** 打印寄存器 | 特权 bug **可观测** |
| **④ 桥梁** | **`SyscallEntry`** · **EAX 编号** | **`0x80000000` 终端输出** |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. Ring 3 与页表 User 位 | [notes/section-2-Ring3与页表User位.md](./notes/section-2-Ring3与页表User位.md) |
| 3. TSS 与 RSP0 内核栈 | [notes/section-3-TSS与RSP0内核栈.md](./notes/section-3-TSS与RSP0内核栈.md) |
| 4. 异常处理与调试 | [notes/section-4-异常处理与调试.md](./notes/section-4-异常处理与调试.md) |
| 5. syscall 机制与 SyscallEntry | [notes/section-5-syscall机制与SyscallEntry.md](./notes/section-5-syscall机制与SyscallEntry.md) |
| 6. 终端打印 syscall 与小结 | [notes/section-6-终端打印syscall与小结.md](./notes/section-6-终端打印syscall与小结.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 本章做了什么？ | **用户态应用** · **syscall 分发** · **安全 printk 到终端** |
| 与 01 川合 OS 对照？ | 01 **Day 20+ syscall**；Mikan **`syscall`/`sysret` + MSR** |
| 与 Linux / CSAPP 对照？ | **08 TLPI** · **05 LKD** — `int 0x80` vs **x86-64 syscall** |

**本章目的：** **护城河（特权级）+ 吊桥（syscall）** — 规范内核/应用交互

---

## 本章学习目标 · 自检

- [ ] **Ring 3** · 页表 **U/S** 如何挡内核
- [ ] **TSS.RSP0** 在 **Ring3→Ring0 中断** 时的作用
- [ ] **`IA32_LSTAR`** · **`SyscallEntry`** · **`syscall_table`**
- [ ] **`0x80000000`** 终端输出验证

---

## 相关

- 上一章：[../chapter-19-paging/](../chapter-19-paging/)
- 下一章：[../chapter-21-window-apps/](../chapter-21-window-apps/)
- 前置：[../chapter-07-interrupt-fifo/](../chapter-07-interrupt-fifo/) · [../chapter-08-memory/](../chapter-08-memory/) · [../chapter-19-paging/](../chapter-19-paging/)
