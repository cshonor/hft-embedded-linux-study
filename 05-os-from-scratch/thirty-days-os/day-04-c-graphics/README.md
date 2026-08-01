# Day 4 · C 语言与画面显示练习


> **原书第四章** · **GUI 开端** — VRAM 写入、**指针**、**调色板**、中断屏蔽、画矩形 → 「像 OS」的任务条。

---

## 和 Day 2 / Day 3 比，第四章到底在干什么？（先读这段）

| | **Day 2** | **Day 3** | **Day 4（本章）** |
|--|-----------|-----------|-------------------|
| **核心任务** | 做启动区、Hello World | 读盘 + 进 32 位 + **C 跑起来** | **在图形屏上画出东西** |
| **屏幕** | 文本模式打字 | **图形模式全黑**（证明 C 写了显存） | **白屏 / 条纹 / 矩形 / 任务条** |
| **改哪份代码** | `helloos.asm` | 四个文件（ipl / nasmhead / bootpack / asmfunc） | 主要改 **`bootpack.c` 的 `HariMain`** |
| **新技能** | 汇编、`INT 0x10` 文本 | 读盘、`INT 0x13`、16→32、C 链接 | **指针写 VRAM**、调色板、画矩形 |

**一句话：** Day 3 让 OS **能跑、能黑屏**；Day 4 教你在 **`0xA0000` 显存上画画** — 从「黑屏」到「有界面雏形」。

### 本章故事线（仍用 Day 3 的启动链，只加厚 `HariMain`）

```text
（启动链与 Day 3 相同：IPL → nasmhead 切 0x13 → HariMain）

Day 4 在 HariMain 里陆续加上：
  ① for 循环写 0xA0000        → 全白 / 黑白条纹（位运算）
  ② char * 指向显存           → 理解「C 写内存 = 画点」
  ③ 调色板 OUT 0x03c8/0x03c9  → 8 位色号真正变成你要的颜色
  ④ CLI/STI 设调色板时不被打断
  ⑤ boxfill 画矩形            → 屏幕底部一条「像任务栏」的色块
```

### QEMU 里本章的验收

| 阶段 | 现象 |
|------|------|
| §4.1 填显存 | **全白** 或 **黑白竖条纹** |
| §4.2 指针 | **左黑右白** |
| §4.3 调色板 | **黑 / 红 / 灰** 横区 |
| §4.4 CLI | **全屏绿** |
| §4.5 画矩形 | 底部 **黑色任务条** |

**不必重写 IPL / nasmhead** — 图形模式 Day 3 已切好；Day 4 **专注 C 怎么操纵帧缓冲**。

代码示例：[code/](./code/)（**§4.1～4.5 各一节一例**，见 [code/README.md](./code/README.md)）

---

### 本节结构（五块）

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① VRAM + 位运算** | `for` 写显存 | 全白屏 · **黑白条纹** |
| **② 指针** | `char *p`、cast | C 操控内存 = 汇编 **`[]` 寻址** |
| **③ 调色板** | 8 位色 · 端口 **`OUT`** | **256 色号 → 16 色** 映射 |
| **④ EFLAGS / 中断** | **`CLI`/`STI`** · **`PUSHFD`/`POPFD`** | 设调色板时不被打断 |
| **⑤ 画矩形** | 点、矩形函数 | 底部 **类任务条** 条带 |

---

## 小节笔记

| 段 | 笔记 | 代码 |
|----|------|------|
| 用 C 写入显存与位运算 | [notes/section-4.1-用-C-写入显存与位运算.md](./notes/section-4.1-用-C-写入显存与位运算.md) | [code/sec-4.1-vram-fill-and-stripes/](./code/sec-4.1-vram-fill-and-stripes/) |
| 挑战并理解指针 | [notes/section-4.2-挑战并理解指针.md](./notes/section-4.2-挑战并理解指针.md) | [code/sec-4.2-vram-pointer-walk/](./code/sec-4.2-vram-pointer-walk/) |
| 调色板（Palette）与色号 | [notes/section-4.3-调色板Palette与色号.md](./notes/section-4.3-调色板Palette与色号.md) | [code/sec-4.3-palette-16-colors/](./code/sec-4.3-palette-16-colors/) |
| EFLAGS 寄存器与中断控制 | [notes/section-4.4-EFLAGS-寄存器与中断控制.md](./notes/section-4.4-EFLAGS-寄存器与中断控制.md) | [code/sec-4.4-cli-sti-palette/](./code/sec-4.4-cli-sti-palette/) |
| 绘制矩形与 OS 雏形 | [notes/section-4.5-绘制矩形与-OS-雏形.md](./notes/section-4.5-绘制矩形与-OS-雏形.md) | [code/sec-4.5-boxfill-taskbar/](./code/sec-4.5-boxfill-taskbar/) |

---

## 本日小结

| 问题 | 答案 |
|------|------|
| 图形模式像素写哪？ | **VRAM `0xa0000` 起**，每字节 1 色号 |
| 条纹怎么来的？ | **`for` + AND/OR/XOR** 处理像素数据 |
| 指针为何必需？ | 指定 **地址 + 字节宽度**，对应汇编 **BYTE 写内存** |
| 8 位色怎么用 RGB？ | **调色板** 映射；**`0x03c8`/`0x03c9` + `OUT`** |
| 设色为何关中断？ | **`CLI`/`STI`**，**`PUSHFD`/`POPFD`** 保 **EFLAGS** |
| 界面里程碑？ | **画矩形** → 底部 **任务条** 雏形 |

**本章定位：** **GUI 起点** — 为后续窗口、鼠标、文字渲染打 ** framebuffer + 调色板 + 绘图原语** 地基。

---

---

## 本日学习目标 · 自检

- [ ] 知道 **VRAM `0xa0000`** 与 **320×200** 坐标换算
- [ ] 能用 **`char *`** 解释 `*p` 与汇编寻址的对应
- [ ] 说清 **8 位索引色** 与 **调色板** 的关系
- [ ] 理解 **`CLI`/`STI`** 与 **`PUSHFD`/`POPFD`** 的用途
- [ ] 找到 **画点 / 画矩形** 在代码中的入口

---

← [Day 3](./day-03-32位模式与导入C语言.md) · [01 导读](../README.md) · [Day 5](./day-05-结构体文字显示与GDT-IDT.md)

---

## 相关

- 上一日：[../day-03-32bit-c/](../day-03-32bit-c/)
- 下一日：[../day-05-gdt-idt/](../day-05-gdt-idt/)
- 模块导读：[../../README.md](../../README.md) · [../../OUTLINE.md](../../OUTLINE.md)
