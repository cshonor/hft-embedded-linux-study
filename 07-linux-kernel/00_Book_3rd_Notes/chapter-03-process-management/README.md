# Ch 3 进程管理 · Process Management

> **Linux Kernel Development 3rd** · Robert Love · **选读**

> 本章定位：Linux **进程抽象** — `task_struct`、状态、`fork`+COW、`clone`、退出与僵尸。为 **Ch 4 调度**、**Ch 15 地址空间** 打底。  
> 拓展⑦⑧：ELF 加载、PID/FD 与 `fork`+`exec` 身份-资源链。

---

## 本节结构

| 节 | 主题 | 带走什么 |
|----|------|----------|
| **① 进程概念** | 执行期程序 + 资源 | **`fork` → `exec`** |
| **② 描述符** | `task_struct` · task list | Slab · `thread_info` |
| **③ 进程状态** | `state` 五态 | 可中断 vs 不可中断睡眠 |
| **④ 创建与 COW** | `fork()` 优化 | **写时拷贝** |
| **⑤ 线程** | 无专用线程类型 | **`clone()` + 标志** · 内核线程 |
| **⑥ 终结** | `exit` · 僵尸 · 孤儿 | **`wait` · reparent → init** |
| **⑦ ELF/exec**（拓展） | ET_* · Program Header | 静态 ELF → 进程映像 |
| **⑧ PID/FD**（拓展） | 身份 vs 资源钥匙 · **fork 共享 `struct file`/offset** | fork 新 PID；exec 换程序 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 进程的概念 | [notes/section-3.1-进程的概念.md](./notes/section-3.1-进程的概念.md) |
| 进程描述符与任务结构 | [notes/section-3.2-进程描述符与任务结构.md](./notes/section-3.2-进程描述符与任务结构.md) |
| 进程状态 | [notes/section-3.3-进程状态.md](./notes/section-3.3-进程状态.md) |
| 进程创建与写时拷贝 | [notes/section-3.4-进程创建与写时拷贝.md](./notes/section-3.4-进程创建与写时拷贝.md) |
| Linux 的线程实现 | [notes/section-3.5-Linux-的线程实现.md](./notes/section-3.5-Linux-的线程实现.md) |
| 进程终结 | [notes/section-3.6-进程终结.md](./notes/section-3.6-进程终结.md) |
| ELF 体系与 exec 加载（拓展） | [notes/section-3.7-ELF体系与exec加载.md](./notes/section-3.7-ELF体系与exec加载.md) |
| 身份 PID 与资源 FD（拓展） | [notes/section-3.8-身份PID与资源FD.md](./notes/section-3.8-身份PID与资源FD.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 进程是什么？ | **执行中程序 + 资源集合** |
| 内核怎么表示？ | **`task_struct`** |
| `fork` / `exec`？ | fork **新 PID** 复制；exec **同 PID** 换 ELF（§3.8） |
| ELF？ | 磁盘模板；`execve`+Program Header 才进进程（§3.7） |
| 线程？ | **`clone` 标志共享** — 无单独线程结构 |

---

## 本章学习目标 · 自检

- [ ] 画出 **`fork` → COW → exec(ELF)`**
- [ ] 区分 **PID（身份）** 与 **FD（钥匙）**
- [ ] 解释僵尸与 `wait`
- [ ] 下接调度：task 进 runqueue（Ch 4）

---

## 相关章节

- 上一章：[../chapter-02-getting-started/](../chapter-02-getting-started/)（含 [§2.5 UEFI/ELF](../chapter-02-getting-started/notes/section-2.5-ELF与UEFI启动链路.md)）
- 下一章：[../chapter-04-process-scheduling/](../chapter-04-process-scheduling/)
- 全书导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
