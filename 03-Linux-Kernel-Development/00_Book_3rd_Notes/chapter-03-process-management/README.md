# Ch 3 进程管理 · Process Management

> **Linux Kernel Development 3rd** · Robert Love · **选读**

> 本章定位：Linux **进程抽象** 的内核实现 — `task_struct`、状态机、`fork`+COW、`clone` 线程模型、退出与僵尸。为 **Ch 4 调度**、**Ch 15 地址空间** 打底。

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

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 进程是什么？ | **执行中程序 + 资源集合** |
| 内核怎么表示？ | **`task_struct`** on **task list** · Slab + **`thread_info`** |
| 五态？ | **RUNNING / INTERRUPTIBLE / UNINTERRUPTIBLE / TRACED / STOPPED** |
| `fork` 为何快？ | **COW** — 写时才复制物理页 |
| 线程？ | **`clone` 标志共享资源** — 无单独线程结构 |
| 内核线程？ | 仅内核态、无用户地址空间、内核创建 |
| 退出后为何还有 PID？ | **僵尸** — 等父 **`wait`**；孤儿由 **init** 收养 |

---

## 本章学习目标 · 自检

- [ ] 画出 **`fork` → COW → exec** 与 **pthread ≈ clone(CLONE_VM|…)** 的关系
- [ ] 区分 **可中断 / 不可中断睡眠** 与信号行为
- [ ] 解释 **僵尸** 占什么、**`wait`** 做什么
- [ ] 知道 **内核线程** 与用户线程不是同一层抽象
- [ ] 能对照 HFT：**少 fork、多线程、注意 D 状态与 kthread 争核**

---

## 相关章节

- 上一章：[../chapter-02-getting-started/](../chapter-02-getting-started/)
- 下一章：[../chapter-04-process-scheduling/](../chapter-04-process-scheduling/)
- 全书导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
