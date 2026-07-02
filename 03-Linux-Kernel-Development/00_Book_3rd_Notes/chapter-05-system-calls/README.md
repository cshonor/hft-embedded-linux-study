# Ch 5 系统调用 · System Calls

> **Linux Kernel Development 3rd** · Robert Love · **选读**

> 本章定位：**用户态 ↔ 内核** 的合法正门 — syscall 号、`sys_call_table`、陷入路径、**参数验证**、进程上下文。HFT **少 syscall、懂延迟从哪来** 的底层一页。

---

## 本节结构

| 节 | 主题 | 带走什么 |
|----|------|----------|
| **① 与内核通信** | API vs syscall | **机制，非策略** |
| **② 基础** | 号 · `sys_*` | **`sys_call_table`** |
| **③ 处理程序** | 陷入 · 寄存器传参 | x86 **`eax`** 号 |
| **④ 实现与安全** | 验证 · capabilities | **`copy_*_user`** |
| **⑤ 上下文** | 进程上下文 | 可睡眠 · 可抢占 · 可重入 |
| **⑥ 添加 syscall** | 绑定与替代 | **慎增新号** |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 与内核通信 | [notes/section-5.1-与内核通信.md](./notes/section-5.1-与内核通信.md) |
| 系统调用基础 | [notes/section-5.2-系统调用基础.md](./notes/section-5.2-系统调用基础.md) |
| 系统调用处理程序 | [notes/section-5.3-系统调用处理程序.md](./notes/section-5.3-系统调用处理程序.md) |
| 实现与参数验证 | [notes/section-5.4-实现与参数验证.md](./notes/section-5.4-实现与参数验证.md) |
| 系统调用上下文 | [notes/section-5.5-系统调用上下文.md](./notes/section-5.5-系统调用上下文.md) |
| 添加系统调用与替代方案 | [notes/section-5.6-添加系统调用与替代方案.md](./notes/section-5.6-添加系统调用与替代方案.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 用户如何进内核？ | **syscall（+ 异常）** — 常经 **libc 包装** |
| 内核怎么分发？ | **号 → `sys_call_table` → `sys_*`** |
| x86 怎么传号？ | **`eax`**（64 位为 `rax` 等） |
| 安全核心？ | **验证指针 + `copy_*_user` + `capable`** |
| 什么上下文？ | **进程上下文** — 可睡眠、可抢占、要可重入 |
| 能随便加 syscall 吗？ | **否** — 优先 **设备节点 / sysfs** 等 |

---

## 本章学习目标 · 自检

- [ ] 区分 **libc API** 与 **底层 syscall**
- [ ] 说出 **`copy_from_user` / `copy_to_user`** 为何必须
- [ ] 对比 **syscall 进程上下文** vs **中断上下文**（Ch 7 不可睡眠）
- [ ] 解释 **号不回收** 与 **`sys_ni_syscall`**
- [ ] 能举 HFT **减 syscall** 手段（`mmap`、批量、旁路）

---

## 相关章节

- 上一章：[../chapter-04-process-scheduling/](../chapter-04-process-scheduling/)
- 下一章：[../chapter-06-kernel-data-structures/](../chapter-06-kernel-data-structures/)
- 全书导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
