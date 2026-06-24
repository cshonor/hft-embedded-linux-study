## ④ 代码整理 · 模块化

`bootpack.c` 再次臃肿 → 按域拆分：

| 新文件 | 内容 |
|--------|------|
| **`window.c`** | 窗口绘制、拖拽相关 |
| **`console.c`** | 命令行、**type/mem/cls/dir** |
| **`file.c`** | **FAT、目录、loadfile** |

**Day 6 模式再现** — 功能涨 → **拆模块 + Makefile**。

---
