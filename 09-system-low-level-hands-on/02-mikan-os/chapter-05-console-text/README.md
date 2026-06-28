# Ch 5 · 文本显示和控制台类

> **原书第 5 章** · HFT **⚪** · 官方源码标签 `osbook_day05`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **为 OS 安上「嘴」：** 字体 · **Console** · **`printk()`**

---

### 本章结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 字体** | 1 bit/像素 · **`WriteAscii()`** | 像素级字符绘制 |
| **② 工程** | `graphics` / `font` 拆分 · **make** 增量编译 | 模块清晰 |
| **③ 嵌入** | 東雲字体 · **objcopy** | 无 FS 时把数据链进内核 |
| **④ 控制台** | **Console** 滚动 · **Newlib `sprintf`** | 格式化日志 |
| **⑤ 全局** | **`printk()`** · `va_list` | 全内核调试输出 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. WriteAscii 与位图字体 | [notes/section-2-WriteAscii与位图字体.md](./notes/section-2-WriteAscii与位图字体.md) |
| 3. 代码拆分与 make | [notes/section-3-代码拆分与make.md](./notes/section-3-代码拆分与make.md) |
| 4. 外部字体嵌入 | [notes/section-4-外部字体嵌入.md](./notes/section-4-外部字体嵌入.md) |
| 5. Console 与 Newlib | [notes/section-5-Console与Newlib.md](./notes/section-5-Console与Newlib.md) |
| 6. printk 与小结 | [notes/section-6-printk与小结.md](./notes/section-6-printk与小结.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 本章做了什么？ | **文本输出** · 滚动 **Console** · 全局 **`printk()`** |
| 与 01 川合 OS 对照？ | 01 **Day 8–9** 文本模式 VGA；Mikan 在 **GOP 像素** 上自绘字体 |
| 与 Linux / CSAPP 对照？ | **`printk`** ≈ 内核早期 `printf` — 对照 [05-LKD](../../../05-Linux-Kernel-Development/) |

**本章目的：** 启动信息与 **全内核调试窗口** — 为 Ch 7 中断、Ch 8 内存等提供可见日志。

---

## 本章学习目标 · 自检

- [ ] 用 **位图 + 位运算** 实现单字符绘制
- [ ] 拆分 **graphics / font** 模块并利用 **make** 增量编译
- [ ] 说清 **objcopy 嵌入字体** 流程
- [ ] 实现 **Console 滚动**（`memcpy` 上移）
- [ ] 会用 **Newlib `sprintf`/`vsprintf`** 并实现 **`printk`**

---

## 相关

- 上一章：[../chapter-04-pixel-make/](../chapter-04-pixel-make/)
- 下一章：[../chapter-06-mouse-pci/](../chapter-06-mouse-pci/)
- 附录：[appendix-F ASCII 表](../appendix-F-ascii-table/)
- 模块导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
