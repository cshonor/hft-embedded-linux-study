# Day 3 · 32 位模式与导入 C 语言


> **原书第三章** · **承前启后** — 真正的 IPL 读盘、**haribote-os** 跑起来、**16 位 → 32 位**、**C 语言** 写 OS 逻辑。

---

## 和 Day 2 比，第三章到底在干什么？（先读这段）

你已经知道 **Day 2**：做一个 **512 字节的启动区**，在 **文本模式** 下用 BIOS 打出 **Hello World**，然后 **`HLT` 停住** —— 扇区里 **只有引导代码**，没有「真正的操作系统」。

**第三章** 把这件事 **升级成能跑起来的迷你 OS 骨架**：

| | **Day 2（第二章）** | **Day 3（第三章）** |
|--|-------------------|-------------------|
| **启动区里有什么** | 只有 **一段汇编**（打印 Hello） | **IPL（512B）** 只负责 **读盘**；真正的 OS 在 **bootpack** 里（几百 KB） |
| **从哪启动** | BIOS 加载 1 个扇区 → `0x7C00` | 同上，但 IPL 再用 **`INT 0x13`** 把 **bootpack 读进内存 `0x8200`** |
| **屏幕上看到什么** | 文本 **Hello World**（`0xB8000`） | 切 **图形模式 0x13**，**全黑屏** = OS 跑起来了 |
| **用什么语言写 OS** | **纯汇编**（扇区装不下 C） | **汇编搭台 + C 唱戏**：`HariMain` 用 **C** 写主逻辑 |
| **CPU 模式** | 一直 **16 位实模式** | **16 位读盘 / 切 VGA** → **32 位保护模式** 跑 C |

**一句话：** Day 2 是「能启动并说话」；Day 3 是「**从软盘读出整个 OS 包、切图形、进 32 位、用 C 当内核入口**」。

### 本章故事线（按时间顺序）

```text
① BIOS 加载 ipl.bin（512B）→ 0x7C00
② IPL：INT 0x13 从软盘读 bootpack → 0x8200，打印 load done，JMP 0x8200
③ nasmhead（汇编）：INT 0x10 切 320×200 图形、开 A20、建 GDT、进 32 位
④ bootpack.c：HariMain 用 C 把显存 0xA0000 填黑 → 黑屏
⑤ asmfunc：C 调 io_hlt() 等汇编小函数
```

### QEMU 里本章的验收

| 阶段 | 现象 |
|------|------|
| 只有 IPL、还没 bootpack | `load done` 后跳空白内存（正常） |
| IPL + bootpack 都写好 | **整屏黑**（不是 Hello 文本了） |

### 工程里多出来的文件（四件套）

| 文件 | 干什么 |
|------|--------|
| **`ipl.asm`** | 读盘搬运工（仍在 512B 里） |
| **`nasmhead.asm`** | 切 VGA、切 32 位、`call HariMain` |
| **`bootpack.c`** | **`HariMain`** — OS 主逻辑入口 |
| **`asmfunc.asm`** | `io_hlt` 等 C 写不了的指令 |

代码目录：[code/README.md](./code/README.md)

---

### 本节四段结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 真正 IPL + 读盘** | `INT 0x13` 读软盘 | **10 柱面 / 180KB** 载入内存 |
| **② 纸娃娃 OS + 图形模式** | 命名 **haribote-os**，`INT 0x10` | **320×200×8bit** 全黑 = 成功 |
| **③ 32 位准备 + C** | BIOS 事在 16 位做完 | **`bootpack.c` / `HariMain`** |
| **④ 汇编 + C 协作** | `asmfunc.asm` → `io_hlt` | C 调 **HLT** 等底层指令 |

---

## 小节笔记

| 段 | 笔记 | 代码 |
|----|------|------|
| 制作真正的 IPL 与读取磁盘 | [notes/section-3.1-制作真正的-IPL-与读取磁盘.md](./notes/section-3.1-制作真正的-IPL-与读取磁盘.md)（[3.1.1](./notes/section-3.1.1-IPL-bootpack与镜像布局.md) · [3.1.2](./notes/section-3.1.2-软盘CHS结构与读盘范围.md) · [3.1.3](./notes/section-3.1.3-INT0x13与ipl代码拆解.md) · [3.1.4](./notes/section-3.1.4-实模式读盘与保护模式切换.md)） | [code/sec-3.1-ipl-int13-disk-load/](./code/sec-3.1-ipl-int13-disk-load/) |
| 纸娃娃操作系统 | [notes/section-3.2-纸娃娃操作系统.md](./notes/section-3.2-纸娃娃操作系统.md)（[3.2.1](./notes/section-3.2.1-VGA模式0x13详解.md)） | [code/sec-3.2-vga-mode-0x13/](./code/sec-3.2-vga-mode-0x13/) |
| 32 位模式前期准备与导入 C 语言 | [notes/section-3.3-32-位模式前期准备与导入-C-语言.md](./notes/section-3.3-32-位模式前期准备与导入-C-语言.md)（[3.3.1](./notes/section-3.3.1-16-32-64分工与Load-vs-Run.md) · [3.3.2](./notes/section-3.3.2-进32位前BIOS与启动链.md) · [3.3.3](./notes/section-3.3.3-32位保护模式与段页.md) · [3.3.4](./notes/section-3.3.4-16-32-64阶梯.md) · [3.3.5](./notes/section-3.3.5-MikanOS与UEFI对照.md) · [3.3.6](./notes/section-3.3.6-引入C与嵌入式HFT.md)） | — |
| 汇编与 C 的结合 | [notes/section-3.4-汇编与-C-的结合.md](./notes/section-3.4-汇编与-C-的结合.md)（[3.4.1](./notes/section-3.4.1-为何C需要汇编包装.md) · [3.4.2](./notes/section-3.4.2-io_hlt与工程分层.md) · [3.4.3](./notes/section-3.4.3-16切32与call-C完整例子.md) · [3.4.4](./notes/section-3.4.4-嵌入式HFT与何时用汇编.md)） | [code/sec-3.4-bootpack-asm-and-c/](./code/sec-3.4-bootpack-asm-and-c/) |

---

## 本日小结

| 问题 | 答案 |
|------|------|
| IPL 今天真正做了什么？ | **`INT 0x13` 读盘**，**180KB / 10 柱面** 进内存，带 **5 次重试** |
| 怎么知道 OS 跑了？ | **`INT 0x10` → 320×200×8**，**全黑屏** |
| 为何先 BIOS 再 32 位？ | 32 位下 **不能调 16 位 BIOS** |
| C 从哪进？ | **`bootpack.c`**，入口 **`HariMain`** |
| C 缺指令怎么办？ | **`asmfunc.asm`**（如 **`io_hlt`**）链接进 C |

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
