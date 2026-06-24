# Day 23 · 图形处理相关


> **原书第二十三章** · **GUI API 大扩展** — **malloc/free（EDX 9/10）**、画点/刷新（11/12）、整数画线（13）、关窗（14）、键盘（15）、**任务↔窗口清理**。

---

### 本节五段结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① malloc** | **EDX=9/10** | .hrb **7.6KB→387B** |
| **② 画点/刷新** | **EDX=11/12** | 句柄 **LSB** 禁自动 refresh |
| **③ 画线** | **EDX=13** | **×1024 + >>10** 整数 Bresenham 类 |
| **④ 键盘** | **EDX=15** | **`walk.hrb`** 方向键 |
| **⑤ 关窗清理** | **EDX=14** · **SHEET→TASK** | **Shift+F1** 也清窗 |

---

## 小节笔记

| 段 | 笔记 |
|----|------|
| 编写 malloc | [notes/section-23.1-编写-malloc.md](./notes/section-23.1-编写-malloc.md) |
| 画点 | [notes/section-23.2-画点.md](./notes/section-23.2-画点.md) |
| 画直线 | [notes/section-23.3-画直线.md](./notes/section-23.3-画直线.md) |
| 键盘输入 | [notes/section-23.4-键盘输入.md](./notes/section-23.4-键盘输入.md) |
| 关闭窗口 | [notes/section-23.5-关闭窗口.md](./notes/section-23.5-关闭窗口.md) |
| Day 23 | [notes/section-23.6-Day-23.md](./notes/section-23.6-Day-23.md) |

---

## 本日小结

| 问题 | 答案 |
|------|------|
| .hrb 为何巨大？ | **静态大数组 → 文件填 0** |
| 怎么缩小？ | **EDX 9/10 运行时 alloc** |
| 画点慢？ | **禁自动刷 + EDX 12 批量刷** |
| 无浮点画线？ | **1024 定点 + >>10** |
| 交互 demo？ | **`walk.hrb`** · **EDX 15** |
| 窗残留？ | **EDX 14** + **sheet→task 强杀清理** |

---

---

## 本日学习目标 · 自检

- [ ] 说清 **静态数组 vs API malloc** 对 .hrb 体积影响
- [ ] 解释 **句柄 LSB** 与 **批量 refresh**
- [ ] 描述 **1024 定点画线** 思路
- [ ] 串 **EDX 15 + walk** 交互环
- [ ] 理解 **task 退出/强杀 → 清 sheet**

---

← [Day 22](./day-22-用C语言编写应用程序.md) · [08-1 导读](../README.md) · [Day 24](./day-24-窗口操作.md)

---

## 相关

- 上一日：[../day-22-c-apps/](../day-22-c-apps/)
- 下一日：[../day-24-window-ops/](../day-24-window-ops/)
- 模块导读：[../../README.md](../../README.md) · [../../OUTLINE.md](../../OUTLINE.md)
