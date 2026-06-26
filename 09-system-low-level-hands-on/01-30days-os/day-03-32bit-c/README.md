# Day 3 · 32 位模式与导入 C 语言


> **原书第三章** · **承前启后** — 真正的 IPL 读盘、**haribote-os** 跑起来、**16 位 → 32 位**、**C 语言** 写 OS 逻辑。

---

### 本节四段结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 真正 IPL + 读盘** | `INT 0x13` 读软盘 | **10 柱面 / 180KB** 载入内存 |
| **② 纸娃娃 OS + 图形模式** | 命名 **haribote-os**，`INT 0x10` | **320×200×8bit** 全黑 = 成功 |
| **③ 32 位准备 + C** | BIOS 事在 16 位做完 | **`bootpack.c` / `HariMain`** |
| **④ 汇编 + C 协作** | `naskfunc.asm` → `io_hlt` | C 调 **HLT** 等底层指令 |

---

## 小节笔记

| 段 | 笔记 |
|----|------|
| 制作真正的 IPL 与读取磁盘 | [notes/section-3.1-制作真正的-IPL-与读取磁盘.md](./notes/section-3.1-制作真正的-IPL-与读取磁盘.md) |
| 纸娃娃操作系统 | [notes/section-3.2-纸娃娃操作系统.md](./notes/section-3.2-纸娃娃操作系统.md) |
| 32 位模式前期准备与导入 C 语言 | [notes/section-3.3-32-位模式前期准备与导入-C-语言.md](./notes/section-3.3-32-位模式前期准备与导入-C-语言.md) |
| 汇编与 C 的结合 | [notes/section-3.4-汇编与-C-的结合.md](./notes/section-3.4-汇编与-C-的结合.md) |

---

## 本日小结

| 问题 | 答案 |
|------|------|
| IPL 今天真正做了什么？ | **`INT 0x13` 读盘**，**180KB / 10 柱面** 进内存，带 **5 次重试** |
| 怎么知道 OS 跑了？ | **`INT 0x10` → 320×200×8**，**全黑屏** |
| 为何先 BIOS 再 32 位？ | 32 位下 **不能调 16 位 BIOS** |
| C 从哪进？ | **`bootpack.c`**，入口 **`HariMain`** |
| C 缺指令怎么办？ | **`naskfunc.asm`**（如 **`io_hlt`**）链接进 C |

**承前启后：** Day 1–2 会 boot + 懂汇编；Day 3 **读盘 + 图形 + C 环境** — 后续高级功能都建在 **`HariMain` 这条链** 上。

---

---

## 本日学习目标 · 自检

- [ ] 说清 **柱面 / 磁头 / 扇区** 与 **`INT 0x13` 读盘**
- [ ] 理解 **读盘重试** 的必要性
- [ ] 知道 **haribote-os** 与 **VGA 320×200** 验收方式
- [ ] 能解释 **进 32 位前必须做完 BIOS 工作** 的原因
- [ ] 找到 **`bootpack.c` / `HariMain` / `io_hlt`** 在工程里的分工

---

← [Day 2](./day-02-汇编语言与Makefile入门.md) · [01 导读](../README.md) · [Day 4](./day-04-C语言与画面显示练习.md)

---

## 相关

- 上一日：[../day-02-asm-makefile/](../day-02-asm-makefile/)
- 下一日：[../day-04-c-graphics/](../day-04-c-graphics/)
- 模块导读：[../../README.md](../../README.md) · [../../OUTLINE.md](../../OUTLINE.md)
