# Day 28 · 文件操作与文字显示


> **原书第二十八章** · **实用化 + 国际化** — **`__alloca`**、文件 API **EDX 21–25**、命令行 **EDX 26**、**nihongo.fnt** 全角 · **langmode** · **EDX 27**。

---

### 本节四段结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① __alloca** | 栈上大数组 | **`winhelo` 7664→174B** |
| **② 文件 API** | **EDX 21–25** | **`typeipl.hrb`** |
| **③ 命令行 API** | **EDX 26** | 外部 **`type.hrb`** 替代内核 type |
| **④ 全角文字** | **`nihongo.fnt`** · **langmode** · **EDX 27** | 16×16 双字节绘制 |

---

## 小节笔记

| 段 | 笔记 |
|----|------|
| 攻克 __alloca | [notes/section-28.1-攻克-__alloca.md](./notes/section-28.1-攻克-__alloca.md) |
| 文件操作 API | [notes/section-28.2-文件操作-API.md](./notes/section-28.2-文件操作-API.md) |
| 命令行 API | [notes/section-28.3-命令行-API.md](./notes/section-28.3-命令行-API.md) |
| 日文全角 | [notes/section-28.4-日文全角.md](./notes/section-28.4-日文全角.md) |
| Day 28 | [notes/section-28.5-Day-28.md](./notes/section-28.5-Day-28.md) |

---

## 本日小结

| 问题 | 答案 |
|------|------|
| __alloca 干嘛？ | **栈上动态大数组** · 减 .hrb 体积 |
| 文件 API？ | **21–25** · typeipl 演示 |
| 内核 type 怎么删？ | **EDX 26** + **type.hrb** |
| 字库放哪？ | **`nihongo.fnt` 外置加载** |
| 全角怎么画？ | **16×16** · **回退 8px 拼接** |
| langmode？ | **0/1/2** · **EDX 27** 查询 |

---

---

## 本日学习目标 · 自检

- [ ] 对比 **`__alloca` vs API malloc**
- [ ] 列举 **EDX 21–27**
- [ ] 说清 **type.hrb 解析命令行** 流程
- [ ] 理解 **外置 .fnt** 与 **双字节寻址**
- [ ] 串 Day 18 FAT → 28 用户态文件 API

---

← [Day 27](./day-27-LDT与库.md) · [01 导读](../README.md) · [Day 29](./day-29-压缩与简单应用程序.md)

---

## 相关

- 上一日：[../day-27-ldt-lib/](../day-27-ldt-lib/)
- 下一日：[../day-29-compression/](../day-29-compression/)
- 模块导读：[../../README.md](../../README.md) · [../../OUTLINE.md](../../OUTLINE.md)
