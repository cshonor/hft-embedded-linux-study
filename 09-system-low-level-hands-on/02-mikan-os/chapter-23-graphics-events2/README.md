# Ch 23 · 图形和事件（2）

> **原书第 23 章** · HFT **⚪** · 官方源码标签 `osbook_day23`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **交互爆发：** **鼠标轨迹/按键 · eye/paint · 定时器重构 · cube · blocks**

---

### 本章结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 鼠标** | **移动事件 · 按键 XOR** | **eye · paint** |
| **② 定时器** | **按任务 ID 投递超时** | **timer · cube 动画** |
| **③ 键盘** | **bitset 按下/松开** | **blocks 游戏** |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. 鼠标移动与 eye 应用 | [notes/section-2-鼠标移动与eye应用.md](./notes/section-2-鼠标移动与eye应用.md) |
| 3. 鼠标按键与 paint 绘图 | [notes/section-3-鼠标按键与paint绘图.md](./notes/section-3-鼠标按键与paint绘图.md) |
| 4. 定时器重构与 timer 命令 | [notes/section-4-定时器重构与timer命令.md](./notes/section-4-定时器重构与timer命令.md) |
| 5. cube 旋转立方体动画 | [notes/section-5-cube旋转立方体动画.md](./notes/section-5-cube旋转立方体动画.md) |
| 6. 键盘升级与 blocks 游戏 | [notes/section-6-键盘升级与blocks游戏.md](./notes/section-6-键盘升级与blocks游戏.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 本章做了什么？ | **鼠标/键盘全事件 ·  per-task 定时器 · 动画/游戏** |
| 与 01 川合 OS 对照？ | 01 **Day 23+ 鼠标/游戏**；Mikan **eye/paint/cube/blocks** |
| 与 Linux / CSAPP 对照？ | **X11/输入子系统** 极简版 — GUI 细节 HFT **⚪ 可跳过** |

**本章目的：** 打通 **鼠标轨迹 · 按键起落 · 精准定时** — 运行 **交互应用与游戏**

---

## 本章学习目标 · 自检

- [ ] 鼠标 **移动事件** 如何送到 **活动窗口任务**
- [ ] **按键位图 XOR** 检测 **按下/松开**
- [ ] 定时器消息带 **任务 ID** · **timer ms 睡眠**
- [ ] **blocks** 方向键 + 空格 **事件循环**

---

## 相关

- 上一章：[../chapter-22-graphics-events1/](../chapter-22-graphics-events1/)
- 下一章：[../chapter-24-multi-terminal/](../chapter-24-multi-terminal/)
- 前置：[../chapter-06-mouse-pci/](../chapter-06-mouse-pci/) · [../chapter-12-keyboard/](../chapter-12-keyboard/) · [../chapter-22-graphics-events1/](../chapter-22-graphics-events1/)
