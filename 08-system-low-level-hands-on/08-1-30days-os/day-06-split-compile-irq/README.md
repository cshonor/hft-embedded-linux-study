# Day 6 · 分割编译与中断处理


> **原书第六章** · **模块化重构** + **硬件中断** — 拆源文件、**LGDT/Ring**、**PIC**、汇编 **IRETD** 桥接 C → 键盘按下能响应。

---

### 本节四段结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 分割编译** | 拆 **`bootpack.c`** · 精简 **Makefile** | **`graphic.c` / `dsctbl.c`** 等模块 |
| **② GDT 细节** | **`LGDT`** · 段属性 | **Ring0 / Ring3** 权限隔离 |
| **③ PIC 初始化** | 主/从 PIC 级联 | 外设中断 **汇总到 CPU** |
| **④ 中断处理 + 栈** | **`IRETD`** 封装 · IRQ1/12 | 按 **A** 键 → 屏幕提示 |

---

## 小节笔记

| 段 | 笔记 |
|----|------|
| 分割源文件与整理 Makefile | [notes/section-6.1-分割源文件与整理-Makefile.md](./notes/section-6.1-分割源文件与整理-Makefile.md) |
| GDT 与段属性 | [notes/section-6.2-GDT-与段属性.md](./notes/section-6.2-GDT-与段属性.md) |
| 初始化 PIC | [notes/section-6.3-初始化-PIC.md](./notes/section-6.3-初始化-PIC.md) |
| 中断处理程序 | [notes/section-6.4-中断处理程序.md](./notes/section-6.4-中断处理程序.md) |

---

## 本日小结

| 问题 | 答案 |
|------|------|
| 代码怎么瘦身？ | **多 `.c` 模块** + Makefile **模式规则** |
| Ring0/3 干嘛？ | **内核特权 vs 应用受限** — GDT 属性 enforcement |
| PIC 干嘛？ | **汇总外设 IRQ**，主/从 **级联** |
| 为何汇编包一层？ | C 只有 **RET**，中断必须 **IRETD** |
| 栈在中断里？ | **PUSH/POP** 保存现场 — **LIFO** |
| 验收？ | **按键 A → 屏幕有反应** |

**承前启后：** Day 5 **GDT/IDT 表** → Day 6 **PIC + 真实 ISR** → Day 7 **键鼠数据与移动**。

---

---

## 本日学习目标 · 自检

- [ ] 能说出 **`graphic.c` / `dsctbl.c`** 一类拆分原则
- [ ] 理解 **`LGDT`** 与 **Ring0 / Ring3**
- [ ] 说清 **主 PIC / 从 PIC（IRQ2 级联）**
- [ ] 画出 **汇编 stub → CALL C → IRETD** 流程
- [ ] 用 **栈 LIFO** 解释 PUSH/POP 顺序

---

← [Day 5](./day-05-结构体文字显示与GDT-IDT.md) · [08-1 导读](../README.md) · [Day 7](./day-07-FIFO与鼠标控制.md)

---

## 相关

- 上一日：[../day-05-gdt-idt/](../day-05-gdt-idt/)
- 下一日：[../day-07-fifo-mouse/](../day-07-fifo-mouse/)
- 模块导读：[../../README.md](../../README.md) · [../../OUTLINE.md](../../OUTLINE.md)
