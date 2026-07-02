# Ch 30 · 额外应用

> **原书第 30 章** · HFT **⚪** · 官方源码标签 `osbook_day30`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **收官 polish：** **FindCommand · more · cat stdin · × 关闭 · tview · gview**

---

### 本章结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 启动** | **FindCommand / PATH** | **`grep` 无需 apps/** |
| **② 终端** | **more · cat stdin · `cat > file`** | 分页 · **快速建文件** |
| **③ GUI** | **× → kWindowClose → kQuit** | **安全关窗** |
| **④ 查看器** | **tview · gview · 64KiB 栈** | 文本/图像 **多媒体** |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. FindCommand 与 PATH 搜索 | [notes/section-2-FindCommand与PATH搜索.md](./notes/section-2-FindCommand与PATH搜索.md) |
| 3. more 命令与管道按键 | [notes/section-3-more命令与管道按键.md](./notes/section-3-more命令与管道按键.md) |
| 4. cat 标准输入与重定向建文件 | [notes/section-4-cat标准输入与重定向建文件.md](./notes/section-4-cat标准输入与重定向建文件.md) |
| 5. 窗口关闭按钮 | [notes/section-5-窗口关闭按钮.md](./notes/section-5-窗口关闭按钮.md) |
| 6. tview、gview 与小结 | [notes/section-6-tview-gview与小结.md](./notes/section-6-tview-gview与小结.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 本章做了什么？ | **PATH 式查找 · more/tview/gview · × 关窗 · cat 增强** |
| 与 02 川合 OS 对照？ | 01 **Day 30 收尾应用**；Mikan **多媒体查看器** |
| 与 Linux / CSAPP 对照？ | **PATH · less/more · getopt** — GUI 细节 HFT **⚪** |

**本章目的：** MikanOS **主体开发句号** — **五脏俱全的现代 OS 雏形**

---

## 本章学习目标 · 自检

- [ ] **FindCommand** 在 **apps/** 解析短名
- [ ] **more** + **管道** 按键路由 **右任务**
- [ ] **`cat > foobar`** 与 **stdin 省略文件名**
- [ ] **GetWindowRegion · kWindowClose**
- [ ] **tview getopt · gview stb_image · 64KiB 栈**

---

## 相关

- 上一章：[../chapter-29-ipc/](../chapter-29-ipc/)
- 下一章：[../chapter-31-road-ahead/](../chapter-31-road-ahead/)
- 前置：[../chapter-25-app-read-file/](../chapter-25-app-read-file/) · [../chapter-29-ipc/](../chapter-29-ipc/) · [../chapter-28-japanese-redirect/](../chapter-28-japanese-redirect/)
