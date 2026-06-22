# Ch 8 异常控制流 · Exceptional Control Flow

> **CSAPP 3rd** · Bryant & O'Neill · **选读 🟡**（Part II）

> 本章定位：**正常 `call/ret` 之外的控制流突变** — 硬件异常、中断、进程、syscall、`fork`/`exec`、信号。HFT 热路径多是 **单进程多线程**，但 **syscall、信号、上下文切换成本** 仍要懂。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 8.1 异常（8.1.1–8.1.3） | [notes/section-8.1-异常.md](./notes/section-8.1-异常.md) |
| 8.2 进程（8.2.1–8.2.5） | [notes/section-8.2-进程.md](./notes/section-8.2-进程.md) |
| 8.3 系统调用错误处理 | [notes/section-8.3-系统调用错误处理.md](./notes/section-8.3-系统调用错误处理.md) |
| 8.4 进程控制（8.4.1–8.4.6） | [notes/section-8.4-进程控制.md](./notes/section-8.4-进程控制.md) |
| 8.5 信号（8.5.1–8.5.7） | [notes/section-8.5-信号.md](./notes/section-8.5-信号.md) |
| 8.6–8.8 非本地跳转、工具与小结 | [notes/section-8.6-8.8-非本地跳转与工具.md](./notes/section-8.6-8.8-非本地跳转与工具.md) |

---

## 大白话 · 本章一条线

> **CPU 不只会顺序执行你的 `main` — 中断、缺页、syscall、信号都会「插队」改控制流。**

```
用户态程序
  → syscall / 异常 → 内核态处理 → 返回用户态
  → 信号异步到达 → 信号处理函数 → 返回被打断点
  → fork 复制地址空间 → 父子并发
```

**HFT 三件事：**

1. **热路径少 syscall、少信号** — 收包用 DPDK/ busy poll；别在 tick 里 `fork`
2. **懂上下文切换代价** — 绑核、少跨进程；线程优于多进程共享内存（→ [Ch 12](../chapter-12-并发编程.md)）
3. **优雅停机** — `SIGTERM`/`SIGINT` 处理、子进程回收、**EINTR** 重试

---

## 本章 Checklist

- [ ] 区分 **中断、陷阱、故障、终止** 四类异常
- [ ] 说出进程 **私有地址空间、用户/内核模式、上下文**
- [ ] 会读 `errno`；syscall 失败必须检查返回值
- [ ] 解释 `fork`/`execve`/`waitpid`；僵尸与孤儿进程
- [ ] 信号：**发送、接收、阻塞**；异步信号安全函数
- [ ] 知道为何信号处理里不宜调 `printf`/`malloc`
- [ ] 了解 `setjmp`/`longjmp` 与 C++ 异常的区别（慎用）

---

## HFT 精读捷径

```
必读：8.1 异常/中断直觉 · 8.2 进程与上下文切换 · 8.3 errno/EINTR
运维/架构：8.5 信号（优雅退出）· 8.4 fork/exec 扫读（理解不用）
热路径服务：多线程 > 多进程；信号处理极简或专用线程 sigwait
8.6 setjmp、8.7 工具 — 选读
```

---

## 相关章节

- 上一章：[../chapter-07-linking/](../chapter-07-linking/)
- 下一章：[../chapter-09-virtual-memory/](../chapter-09-virtual-memory/)
- 并发：[../chapter-12-并发编程.md](../chapter-12-并发编程.md)
- OS 专章：[02-SysPerf Ch 3](../../02-Systems-Performance-2nd/chapter-03-operating-systems/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
