# Day 18 · dir 命令


> **原书第十八章** · **Shell 活起来** — FIFO **光标 2/3**、回车滚动 **`cons_newline`**、**`strcmp`** · **mem/cls/dir**、**FAT12** 根目录。

---

### 本节四段结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 光标闪烁** | Tab 时 FIFO **2/3** | **仅焦点窗** 闪光标 |
| **② 回车滚动** | ASCII **10** · **`cons_newline`** | 上移 **16px** 滚屏 |
| **③ mem / cls** | **`<string.h>` `strcmp`** | 按需查内存 · 清屏 |
| **④ dir** | **FAT12 根目录 @ ~0x002600** | 224×32B 目录项 |

---

## 小节笔记

| 段 | 笔记 |
|----|------|
| 智能控制光标闪烁 | [notes/section-18.1-智能控制光标闪烁.md](./notes/section-18.1-智能控制光标闪烁.md) |
| 回车键 | [notes/section-18.2-回车键.md](./notes/section-18.2-回车键.md) |
| mem 与 cls | [notes/section-18.3-mem-与-cls.md](./notes/section-18.3-mem-与-cls.md) |
| dir 命令 | [notes/section-18.4-dir-命令.md](./notes/section-18.4-dir-命令.md) |

---

## 本日小结

| 问题 | 答案 |
|------|------|
| 非焦点为何不闪光标？ | FIFO **2/3** 跨任务通知 |
| 回车做什么？ | **ASCII 10** · y++ |
| 滚屏怎么做？ | **`cons_newline`** 上移 **16px** + 底行涂黑 |
| 命令怎么判？ | **`strcmp`** |
| `mem` / `cls`？ | 指针传 **memman** · 黑矩形清屏 |
| `dir` 读哪？ | **FAT12 根目录 ~0x002600** · **224×32B** |
| 里程碑？ | **交互式 Shell** → Day 19 **执行 .hrb** |

---

---

## 本日学习目标 · 自检

- [ ] 说清 **FIFO 2/3** 与 Tab 焦点配合
- [ ] 描述 **`cons_newline`** 滚屏原理
- [ ] 会用 **`strcmp`** 对比手写字符比较
- [ ] 背出 **FAT12 目录项 32B 布局**（8+3 名/ext）
- [ ] 串起 Day 9 内存 → Day 17 Console → Day 18 命令

---

← [Day 17](./day-17-命令行窗口.md) · [01 导读](../README.md) · [Day 19](./day-19-应用程序.md)

---

## 相关

- 上一日：[../day-17-console/](../day-17-console/)
- 下一日：[../day-19-apps/](../day-19-apps/)
- 模块导读：[../../README.md](../../README.md) · [../../OUTLINE.md](../../OUTLINE.md)
