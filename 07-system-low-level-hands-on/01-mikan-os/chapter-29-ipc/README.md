# Ch 29 · 应用间通信

> **原书第 29 章** · HFT **🟡** · 官方源码标签 `osbook_day29`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **IPC：** **`echo $?` · 管道 `|` · PipeDescriptor · sort · 共享内存**

---

### 本章结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 退出码** | **last_exit_code_ · `echo $?`** | 隐式保存 |
| **② 管道** | **`\|` · PipeDescriptor · kPipe** | **左右并发** |
| **③ 同步** | **WaitFinish/Finish** | 子任务 **生命周期** |
| **④ 进阶** | **sort · cat 行读 · 共享内存** | **管道组合 · 数据竞争** |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. 退出码与 echo $? | [notes/section-2-退出码与echo-question.md](./notes/section-2-退出码与echo-question.md) |
| 3. 管道机制与 PipeDescriptor | [notes/section-3-管道机制与PipeDescriptor.md](./notes/section-3-管道机制与PipeDescriptor.md) |
| 4. WaitFinish 与任务同步 | [notes/section-4-WaitFinish与任务同步.md](./notes/section-4-WaitFinish与任务同步.md) |
| 5. sort、cat 优化与终端修复 | [notes/section-5-sort-cat优化与终端修复.md](./notes/section-5-sort-cat优化与终端修复.md) |
| 6. 共享内存与小结 | [notes/section-6-共享内存与小结.md](./notes/section-6-共享内存与小结.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 本章做了什么？ | **管道 `\|`** · **PipeDescriptor** · **sort** · **共享内存概念** |
| 与 02 川合 OS 对照？ | 01 **Day 29+ IPC**；Mikan **消息队列 kPipe** |
| 与 Linux / CSAPP 对照？ | **08 TLPI pipe** · **CSAPP Ch12 进程间通信** |

**本章目的：** 孤立应用 → **管道连结的有机整体** — CLI **成熟协作**

---

## 本章学习目标 · 自检

- [ ] **`echo $?`** 与 **last_exit_code_**
- [ ] **`\|`** 解析 · **左右 Task 并发**
- [ ] **PipeDescriptor · Message::kPipe**
- [ ] **WaitFinish/Finish** · **共享内存 vs 管道**

---

## 相关

- 上一章：[../chapter-28-japanese-redirect/](../chapter-28-japanese-redirect/)
- 下一章：[../chapter-30-extra-apps/](../chapter-30-extra-apps/)
- 前置：[../chapter-26-app-write-file/](../chapter-26-app-write-file/) · [chapter-28-japanese-redirect/](../chapter-28-japanese-redirect/) · [../chapter-14-multitask2/](../chapter-14-multitask2/)
