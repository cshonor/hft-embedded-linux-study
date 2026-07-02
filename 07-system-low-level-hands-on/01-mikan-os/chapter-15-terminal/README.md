# Ch 15 · 终端

> **原书第 15 章** · HFT **⚪** · 官方源码标签 `osbook_day15`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **窗口体系成熟：** **kLayer 消息** · **活动窗口** · **TaskTerminal** · **DrawArea**

---

### 本章结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 安全绘制** | **kLayer** 消息 · **主任务统一** 图层 | 消除拖窗 **残影** |
| **② 活动窗** | **ActiveLayer** · **ToplevelWindow** | 焦点 · 标题栏 · 键盘路由 |
| **③ 终端** | **TaskTerminal** · **Terminal** · 闪烁光标 | 黑底白字独立 Task |
| **④ 优化** | **DrawArea** 局部重绘 **7×15** | 光标闪 **~16×** |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. kLayer 消息与主任务统一绘制 | [notes/section-2-kLayer消息与主任务统一绘制.md](./notes/section-2-kLayer消息与主任务统一绘制.md) |
| 3. ActiveLayer 与 ToplevelWindow | [notes/section-3-ActiveLayer与ToplevelWindow.md](./notes/section-3-ActiveLayer与ToplevelWindow.md) |
| 4. TaskTerminal 与 Terminal | [notes/section-4-TaskTerminal与Terminal.md](./notes/section-4-TaskTerminal与Terminal.md) |
| 5. DrawArea 局部重绘 | [notes/section-5-DrawArea局部重绘.md](./notes/section-5-DrawArea局部重绘.md) |
| 6. 小结与索引 | [notes/section-6-小结与索引.md](./notes/section-6-小结与索引.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 本章做了什么？ | **规范窗口管理** + **终端 Task** + **安全合成** |
| 与 02 川合 OS 对照？ | 01 命令行较简；Mikan **GUI 终端 + 活动窗** |
| 与 Linux / CSAPP 对照？ | 焦点/TTY 雏形 — 为 **Ch16 命令** 铺路 |

**本章目的：** 终端 **外观与架构就绪** → **Ch16 可敲命令执行**

---

## 本章学习目标 · 自检

- [ ] 说清 **kLayer** 为何由 **主任务** 统一绘制
- [ ] 描述 **ActiveLayer / ToplevelWindow** 与键盘路由
- [ ] 解释 **TaskTerminal** 与 **0.5s 光标闪烁**
- [ ] 会用 **DrawArea** 做 **7×15** 局部重绘

---

## 相关

- 上一章：[../chapter-14-multitask2/](../chapter-14-multitask2/)
- 下一章：[../chapter-16-commands/](../chapter-16-commands/)
- 前置：[../chapter-09-layers/](../chapter-09-layers/) · [../chapter-12-keyboard/](../chapter-12-keyboard/)
