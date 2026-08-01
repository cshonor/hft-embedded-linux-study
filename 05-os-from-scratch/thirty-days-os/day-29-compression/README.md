# Day 29 · 压缩与简单应用程序


> **原书第二十九章** · **tek 解压 + 标准库 + 图形 demo + Invader** — 全角 refresh 修、**`file_loadfile2`**、**stdio 封装**、透明窗 **bball**、**2.28KB 外星人**。

---

### 本节五段结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 全角 bug** | refresh **x-8** | 右半缺失 **修复** |
| **② tek 解压** | **`tek.c` / loadfile2** | **nihongo 142→56.6KB** |
| **③ 标准函数** | **putchar/printf/exit/malloc** | C **通用编程** |
| **④ bball** | **色号 255 透明** | 非矩形窗 · 越界画线修 |
| **⑤ Invader** | 射击游戏 | **.hrb ~2.28KB** |

---

## 小节笔记

| 段 | 笔记 |
|----|------|
| 修复全角字符显示 Bug | [notes/section-29.1-修复全角字符显示-Bug.md](./notes/section-29.1-修复全角字符显示-Bug.md) |
| tek 解压缩 | [notes/section-29.2-tek-解压缩.md](./notes/section-29.2-tek-解压缩.md) |
| 封装 C 语言标准函数 | [notes/section-29.3-封装-C-语言标准函数.md](./notes/section-29.3-封装-C-语言标准函数.md) |
| 非矩形窗口 | [notes/section-29.4-非矩形窗口.md](./notes/section-29.4-非矩形窗口.md) |
| Invader | [notes/section-29.5-Invader.md](./notes/section-29.5-Invader.md) |

---

## 本日小结

| 问题 | 答案 |
|------|------|
| 全角只显右半？ | refresh **`x-8`** |
| 磁盘怎么省？ | **tek** · **loadfile2 自动解压** |
| nihongo 省多少？ | **142→56.6KB** |
| C 怎么好写？ | **printf/malloc/exit** 标准层 |
| 非矩形窗？ | **透明色 255** |
| 里程碑 demo？ | **bball** · **Invader 2.28KB** |

---

---

## 本日学习目标 · 自检

- [ ] 说清 **全角第二字节 refresh 为何要 x-8**
- [ ] 描述 **tek 三目标平衡** 与 **loadfile2**
- [ ] 区分 **API vs 标准函数封装**
- [ ] 知道 **255 透明** 与非矩形窗关系
- [ ] 串 Day 28 字库 → 29 压缩 → 游戏

---

← [Day 28](./day-28-文件操作与文字显示.md) · [01 导读](../README.md) · [Day 30](./day-30-高级的应用程序.md)

---

## 相关

- 上一日：[../day-28-files/](../day-28-files/)
- 下一日：[../day-30-advanced-apps/](../day-30-advanced-apps/)
- 模块导读：[../../README.md](../../README.md) · [../../OUTLINE.md](../../OUTLINE.md)
