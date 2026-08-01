# Day 24 · 窗口操作


> **原书第二十四章** · **桌面级交互** — **F11/Tab/key_win**、**点击激活**、拖标题栏、**× 关窗杀进程**、**EDX 16–19 定时器**、**noodle.hrb**、退出 **清定时器**。

---

### 本节四段结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 层级与焦点** | **F11** 置顶 · **Tab/key_win** | **点击窗口** 激活 |
| **② 拖移与关闭** | **sheet_slide** · **×** | 关窗 **结束 app** |
| **③ 定时器 API** | **EDX 16–19** | **`noodle.hrb`** 3 分钟 |
| **④ 清理定时器** | app 结束 **取消遗留 timer** | 防 Console **乱码** |

---

## 小节笔记

| 段 | 笔记 |
|----|------|
| 窗口层级 | [notes/section-24.1-窗口层级.md](./notes/section-24.1-窗口层级.md) |
| 鼠标拖拽 | [notes/section-24.2-鼠标拖拽.md](./notes/section-24.2-鼠标拖拽.md) |
| 定时器 API | [notes/section-24.3-定时器-API.md](./notes/section-24.3-定时器-API.md) |
| 应用结束 | [notes/section-24.4-应用结束.md](./notes/section-24.4-应用结束.md) |
| Day 24 | [notes/section-24.5-Day-24.md](./notes/section-24.5-Day-24.md) |

---

## 本日小结

| 问题 | 答案 |
|------|------|
| 多窗谁在前？ | **F11 底→顶** · **点击/Tab → key_win** |
| 怎么拖窗？ | **标题栏 + sheet_slide** |
| × 做什么？ | **关窗 + 结束 app** |
| app 定时器？ | **EDX 16–19** · **noodle.hrb** |
| 乱码从哪来？ | 死 timer → **错投 Console FIFO** |
| 怎么修？ | **app 结束清该 task 全部 timer** |

**体验里程碑：** **拖、点、切、关、计时** — 接近 **完整桌面 WM**。

---

---

## 本日学习目标 · 自检

- [ ] 区分 **F11 置顶** vs **Tab 轮转 key_win**
- [ ] 说清 **点击激活 = Z 序 + 焦点**
- [ ] 串 **× → 杀 app → sheet+timer 清理**
- [ ] 列举 **EDX 16–19** 四步
- [ ] 解释 **提前关 noodle 为何需 cancel timer**

---

← [Day 23](./day-23-图形处理相关.md) · [01 导读](../README.md) · [Day 25](./day-25-增加命令行窗口.md)

---

## 相关

- 上一日：[../day-23-graphics/](../day-23-graphics/)
- 下一日：[../day-25-multi-console/](../day-25-multi-console/)
- 模块导读：[../../README.md](../../README.md) · [../../OUTLINE.md](../../OUTLINE.md)
