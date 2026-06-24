# 《30 天自制操作系统》· 学习大纲

> **裁剪说明：** 🔴 必做 · 🟡 建议 · ⚪ 可跳过（HFT 时间紧时）  
> **笔记目录：** [notes/](./notes/)

| Day | 原书主题 | 标签 | 笔记 |
|-----|----------|------|------|
| **1** | 从计算机结构到汇编程序入门 | 🔴 | [day-01](./notes/day-01-从计算机结构到汇编入门.md) |
| **2** | 汇编语言学习与 Makefile 入门 | 🔴 | [day-02](./notes/day-02-汇编语言与Makefile入门.md) |
| **3** | 进入 32 位模式并导入 C 语言 | 🔴 | [day-03](./notes/day-03-32位模式与导入C语言.md) |
| **4** | C 语言与画面显示的练习 | 🔴 | [day-04](./notes/day-04-C语言与画面显示练习.md) |
| **5** | 结构体、文字显示与 GDT/IDT 初始化 | 🔴 | [day-05](./notes/day-05-结构体文字显示与GDT-IDT.md) |
| **6** | 分割编译与中断处理 | 🔴 | [day-06](./notes/day-06-分割编译与中断处理.md) |
| **7** | FIFO 与鼠标控制 | 🔴 | [day-07](./notes/day-07-FIFO与鼠标控制.md) |
| **8** | 鼠标控制与 32 位模式切换 | 🔴 | [day-08](./notes/day-08-鼠标控制与32位模式切换.md) |
| **9** | 内存管理 | 🔴 | [day-09](./notes/day-09-内存管理.md) |
| **10** | 叠加处理 | 🔴 | [day-10](./notes/day-10-叠加处理.md) |
| 11–15 | 窗口、定时器、多任务等 | 🔴 | — |
| 16–23 | 内存分配、页表、多任务 | 🔴 | — |
| 24–30 | 文件、API、Shell | 🟡 | — |

---

## Day 1 要点速览

- 二进制编辑器 → **1,474,560 B** `helloos.img` → **hello, world**
- CPU 只认 **0/1**；汇编 + **`nask.exe`** 生成同一映像
- 引导扇区 **512 B**，末尾 **`55 AA`**；汇编 **`$`** 自动 padding
- **IPL** · **Boot（bootstrap）**

→ 详读 [notes/day-01-从计算机结构到汇编入门.md](./notes/day-01-从计算机结构到汇编入门.md)

---

## Day 2 要点速览

- **`ORG` / `JMP` / `MOV`** · 16 位寄存器 · **`[]` 访存**（慢于寄存器）
- **`INT` → BIOS** 做早期 I/O；引导扇区加载到 **`0x7c00`**
- 只编 **`ipl.bin`（512 B）**，整盘用映像工具拼
- **`Makefile` + `make`** 替代一堆 `.bat`

→ 详读 [notes/day-02-汇编语言与Makefile入门.md](./notes/day-02-汇编语言与Makefile入门.md)

---

## Day 3 要点速览

- 真正 **IPL**：**`INT 0x13`** 读盘，**10 柱面 / 180KB**，**重试 5 次**
- **haribote-os** · **`INT 0x10` → 320×200×8** 全黑 = OS 运行
- **32 位前** 做完 BIOS；**`bootpack.c` / `HariMain`**
- **`naskfunc.nas`** · **`io_hlt`** — 汇编 + C 链接

→ 详读 [notes/day-03-32位模式与导入C语言.md](./notes/day-03-32位模式与导入C语言.md)

---

## Day 4 要点速览

- **VRAM `0xa0000`** · **`for` + 位运算** → 白屏 / 条纹
- **`char *p` / `*p`** = 字节宽内存写
- **调色板** · 端口 **`0x03c8`/`0x03c9`** · **`OUT`**
- **`CLI`/`STI`** · **`PUSHFD`/`POPFD`**（EFLAGS）
- **画矩形** → 底部任务条 · 内核 ~**1.2KB**

→ 详读 [notes/day-04-C语言与画面显示练习.md](./notes/day-04-C语言与画面显示练习.md)

---

## Day 5 要点速览

- **`BOOTINFO` struct** · **`->`** 动态读启动画面信息
- **8×16 点阵** · 外部字体 · **`sprintf`** 屏幕调试
- **16×16 鼠标箭头** · 透明背景
- **GDT** 分段 · **IDT** 0~255 中断 → 处理函数

→ 详读 [notes/day-05-结构体文字显示与GDT-IDT.md](./notes/day-05-结构体文字显示与GDT-IDT.md)

---

## Day 6 要点速览

- 拆 **`graphic.c` / `dsctbl.c`** · Makefile **模式规则**
- **`LGDT`** · **Ring0 / Ring3** 段属性
- **PIC** 主/从级联 · 端口初始化
- **`_asm_inthandler` + C + `IRETD`** · **栈 LIFO**
- **IRQ1→0x21** 键盘 · 按 **A** 屏幕有提示

→ 详读 [notes/day-06-分割编译与中断处理.md](./notes/day-06-分割编译与中断处理.md)

---

## Day 7 要点速览

- 端口 **`0x0060`** · 扫描码 **1E/9E** · ISR **只入队**
- **环形 FIFO** · **`FIFO8`**（替代移位版）
- 鼠标经 **键盘控制器** **`0xFA`** · **IRQ12** · **128B FIFO**
- 晃动鼠标 → **hex 瀑布**（指针 Day 8）

→ 详读 [notes/day-07-FIFO与鼠标控制.md](./notes/day-07-FIFO与鼠标控制.md)

---

## Day 8 要点速览

- **`MOUSE_DEC`** · 3 字节包 · **`&0xC8==0x08`** 同步
- **mx/my** · 边界 · 擦旧画新 · 任务栏 **无图层** 伏笔
- **A20GATE** · **CR0** · **bootpack @ 0x00280000** · **far-JMP → C**
- Day 3–8 **硬件初始化** 贯通

→ 详读 [notes/day-08-鼠标控制与32位模式切换.md](./notes/day-08-鼠标控制与32位模式切换.md)

---

## Day 9 要点速览

- 探针 **0xaa55aa55** · **CR0 关 Cache** 再测容量
- C memtest 被 **优化删循环** → **汇编 memtest_sub**
- 位图 3GB 要 **~768KB** 元数据 → **MEMMAN 空闲块列表**
- **alloc / free** · **合并相邻空闲块**

→ 详读 [notes/day-09-内存管理.md](./notes/day-09-内存管理.md)
