# Day 17 · 命令行窗口


> **原书第十七章** · **Console 雏形** — **idle 任务**、**`console_task`**、Tab 焦点、**每任务 FIFO**、Shift/CapsLock、**键盘 LED**。

---

### 本节五段结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 闲置 + Console** | **`console_task`** 独立任务 | **Idle** 最低优先级 |
| **② Tab 焦点** | **`key_to`** | 标题栏 **灰/亮** 切换 |
| **③ 字符输入** | **TASK 内 FIFO** | Backspace **= 8** |
| **④ Shift/Caps** | **`keytable0/1`** · **`key_shift`** | 符号 + 大小写 **`±0x20`** |
| **⑤ LED** | 端口 **`0x60`** · **`0xED`** | Caps/Num/Scroll 灯 |

---

## 小节笔记

| 段 | 笔记 |
|----|------|
| 闲置任务 | [notes/section-17.1-闲置任务.md](./notes/section-17.1-闲置任务.md) |
| Tab 键 | [notes/section-17.2-Tab-键.md](./notes/section-17.2-Tab-键.md) |
| 字符输入 | [notes/section-17.3-字符输入.md](./notes/section-17.3-字符输入.md) |
| Shift 与 CapsLock | [notes/section-17.4-Shift-与-CapsLock.md](./notes/section-17.4-Shift-与-CapsLock.md) |
| 键盘 LED | [notes/section-17.5-键盘-LED.md](./notes/section-17.5-键盘-LED.md) |

---

## 本日小结

| 问题 | 答案 |
|------|------|
| Console 怎么跑？ | 独立 **`console_task`** · 多任务并发 |
| 没事干跑谁？ | **Idle** 最低优先级 |
| 焦点怎么切？ | **Tab** · **`key_to`** · 标题栏灰/亮 |
| 键送给谁？ | **激活任务的 TASK.fifo** |
| 符号/大小写？ | **`keytable0/1` + key_shift** · **Caps ±0x20** |
| LED？ | **`0x60` · `0xED`** 设 Caps/Num/Scroll |
| 里程碑？ | **可交互 CLI 雏形** → 后续 **跑命令/程序** |

---

---

## 本日学习目标 · 自检

- [ ] 说清 **`console_task` vs HariMain`** 分工
- [ ] 描述 **Tab + key_to + 标题栏反馈**
- [ ] 画出 **scancode → 路由 → 任务 fifo → Console**
- [ ] 解释 **双 keytable** 与 **CapsLock/0x20**
- [ ] 知道 **0xED LED 命令** 与读 scancode 同端口不同语义

---

← [Day 16](./day-16-多任务2.md) · [08-1 导读](../README.md) · [Day 18](./day-18-dir命令.md)

---

## 相关

- 上一日：[../day-16-multitask2/](../day-16-multitask2/)
- 下一日：[../day-18-dir/](../day-18-dir/)
- 模块导读：[../../README.md](../../README.md) · [../../OUTLINE.md](../../OUTLINE.md)
