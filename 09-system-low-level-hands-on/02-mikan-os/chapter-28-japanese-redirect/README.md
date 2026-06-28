# Ch 28 · 日文显示和重定向

> **原书第 28 章** · HFT **⚪** · 官方源码标签 `osbook_day28`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **i18n + shell：** **UTF-8 · FreeType · 32KiB 栈 · `>` 重定向 · PrintToFD**

---

### 本章结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 编码** | **UTF-8 · 半角/全角** | 日文 **光标/换行** |
| **② 渲染** | **FreeType · nihongo.ttf** | 矢量字体 · **32MiB 卷** |
| **③ 稳定** | **栈 4KiB→32KiB** | FreeType **不溢出** |
| **④ 重定向** | **`>` · PrintToFD** | **echo > file** |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. UTF-8 解析与排版 | [notes/section-2-UTF-8解析与排版.md](./notes/section-2-UTF-8解析与排版.md) |
| 3. FreeType 与矢量字体 | [notes/section-3-FreeType与矢量字体.md](./notes/section-3-FreeType与矢量字体.md) |
| 4. 栈扩容与 FreeType Bug | [notes/section-4-栈扩容与FreeType-Bug.md](./notes/section-4-栈扩容与FreeType-Bug.md) |
| 5. 标准输出重定向 | [notes/section-5-标准输出重定向.md](./notes/section-5-标准输出重定向.md) |
| 6. PrintToFD 重构与小结 | [notes/section-6-PrintToFD重构与小结.md](./notes/section-6-PrintToFD重构与小结.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 本章做了什么？ | **UTF-8 日文 · FreeType** · **`>` 重定向** · **PrintToFD** |
| 与 01 川合 OS 对照？ | 01 **Day 28+ 日文/重定向**；Mikan **nihongo.ttf + fd 替换** |
| 与 Linux / CSAPP 对照？ | **locale/i18n** · **shell redirect** — HFT **⚪ 可跳过日文细节** |

**本章目的：** **跨越语言障碍** + **解耦命令输出通道** — CLI **更灵活**

---

## 本章学习目标 · 自检

- [ ] **CountUTF8Size / ConvertUTF8To32** 流程
- [ ] **IsHankaku** 与 **光标宽度**
- [ ] **`>`** 如何替换 **files_[1]**
- [ ] **PrintToFD** vs 直接 **Terminal 画字**

---

## 相关

- 上一章：[../chapter-27-app-memory/](../chapter-27-app-memory/)
- 下一章：[../chapter-29-ipc/](../chapter-29-ipc/)
- 前置：[../chapter-05-console-text/](../chapter-05-console-text/) · [../chapter-17-filesystem/](../chapter-17-filesystem/) · [chapter-26-app-write-file/](../chapter-26-app-write-file/)
