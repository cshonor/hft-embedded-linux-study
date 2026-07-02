# Day 25 · 增加命令行窗口


> **原书第二十五章** · **多 Console + 段隔离** — **PIT 蜂鸣器**、**216 色 + 抖动**、**cons[]**、**task 内 cons/ds_base**、**sel 映射段号**、删 **task_a**。

---

### 本节四段结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 蜂鸣器与色彩** | **PIT BEEP** · **6×6×6 RGB** · **抖动** | `beepup/down` · **21 级** 渐变感 |
| **② 多 Console** | **`cons[]` 数组** | **TASK 存 cons + ds_base** |
| **③ 段冲突** | **sel → 1003~2002 / 2003~3002** | 并发 app **独立段** |
| **④ 删 task_a** | 去掉测试花瓶窗 | 修 **`key_win==0`** 重启 bug |

---

## 小节笔记

| 段 | 笔记 |
|----|------|
| 蜂鸣器发声 | [notes/section-25.1-蜂鸣器发声.md](./notes/section-25.1-蜂鸣器发声.md) |
| 支持多个命令行窗口 | [notes/section-25.2-支持多个命令行窗口.md](./notes/section-25.2-支持多个命令行窗口.md) |
| 内存段冲突 | [notes/section-25.3-内存段冲突.md](./notes/section-25.3-内存段冲突.md) |
| 删除 task_a | [notes/section-25.4-删除-task_a.md](./notes/section-25.4-删除-task_a.md) |

---

## 本日小结

| 问题 | 答案 |
|------|------|
| 怎么发声？ | **PIT 控蜂鸣器** · **beep API** |
| 颜色怎么多？ | **6×6×6 RGB** + **抖动 → ~21 级感** |
| 多 Console 关键？ | **`cons[]` + TASK.cons/ds_base** |
| 并发段冲突？ | **`sel` → 1003~2002 / 2003~3002** |
| task_a？ | **删掉** · 修 **key_win==0** |
| 里程碑？ | **多 Console + 多 app 真并行** |

---

---

## 本日学习目标 · 自检

- [ ] 说清 **蜂鸣器与 PIT** 关系
- [ ] 解释 **抖动** 为何能「增阶」
- [ ] 描述 **TASK.cons** 修「输出串窗」
- [ ] 理解 **动态段号** vs Day 21 固定 1003/1004
- [ ] 知道删 **task_a** 与 **key_win 判空**

---

← [Day 24](./day-24-窗口操作.md) · [01 导读](../README.md) · [Day 26](./day-26-为窗口移动提速.md)

---

## 相关

- 上一日：[../day-24-window-ops/](../day-24-window-ops/)
- 下一日：[../day-26-window-speed/](../day-26-window-speed/)
- 模块导读：[../../README.md](../../README.md) · [../../OUTLINE.md](../../OUTLINE.md)
