# 《30 天自制操作系统》· 学习大纲

> **裁剪说明：** 🔴 必做 · 🟡 建议 · ⚪ 可跳过（HFT 时间紧时）  
> **笔记目录：** 各 Day 独立目录 `day-XX-slug/`（导读 `README.md` + `notes/section-*.md`）

| Day | 原书主题 | 标签 | 笔记 |
|-----|----------|------|------|
| **1** | 从计算机结构到汇编程序入门 | 🔴 | [day-01](./day-01-boot-asm/) |
| **2** | 汇编语言学习与 Makefile 入门 | 🔴 | [day-02](./day-02-asm-makefile/) |
| **3** | 进入 32 位模式并导入 C 语言 | 🔴 | [day-03](./day-03-32bit-c/) |
| **4** | C 语言与画面显示的练习 | 🔴 | [day-04](./day-04-c-graphics/) |
| **5** | 结构体、文字显示与 GDT/IDT 初始化 | 🔴 | [day-05](./day-05-gdt-idt/) |
| **6** | 分割编译与中断处理 | 🔴 | [day-06](./day-06-split-compile-irq/) |
| **7** | FIFO 与鼠标控制 | 🔴 | [day-07](./day-07-fifo-mouse/) |
| **8** | 鼠标控制与 32 位模式切换 | 🔴 | [day-08](./day-08-mouse-32bit/) |
| **9** | 内存管理 | 🔴 | [day-09](./day-09-memory/) |
| **10** | 叠加处理 | 🔴 | [day-10](./day-10-layers/) |
| **11** | 制作窗口 | 🔴 | [day-11](./day-11-window/) |
| **12** | 定时器（1） | 🔴 | [day-12](./day-12-timer1/) |
| **13** | 定时器（2） | 🔴 | [day-13](./day-13-timer2/) |
| **14** | 高分辨率及键盘输入 | 🔴 | [day-14](./day-14-keyboard/) |
| **15** | 多任务（1） | 🔴 | [day-15](./day-15-multitask1/) |
| **08** | 多任务（2） | 🔴 | [day-16](./day-16-multitask2/) |
| **17** | 命令行窗口 | 🔴 | [day-17](./day-17-console/) |
| **18** | dir 命令 | 🔴 | [day-18](./day-18-dir/) |
| **19** | 应用程序 | 🔴 | [day-19](./day-19-apps/) |
| **20** | API | 🔴 | [day-20](./day-20-api/) |
| **21** | 保护操作系统 | 🔴 | [day-21](./day-21-protection/) |
| **22** | 用 C 语言编写应用程序 | 🔴 | [day-22](./day-22-c-apps/) |
| **23** | 图形处理相关 | 🔴 | [day-23](./day-23-graphics/) |
| **24** | 窗口操作 | 🔴 | [day-24](./day-24-window-ops/) |
| **25** | 增加命令行窗口 | 🔴 | [day-25](./day-25-multi-console/) |
| **26** | 为窗口移动提速 | 🔴 | [day-26](./day-26-window-speed/) |
| **27** | LDT 与库 | 🔴 | [day-27](./day-27-ldt-lib/) |
| **28** | 文件操作与文字显示 | 🔴 | [day-28](./day-28-files/) |
| **29** | 压缩与简单的应用程序 | 🔴 | [day-29](./day-29-compression/) |
| **30** | 高级的应用程序 | 🔴 | [day-30](./day-30-advanced-apps/) |

---

## Day 1 要点速览

- 二进制编辑器 → **1,474,560 B** `helloos.img` → **hello, world**
- CPU 只认 **0/1**；**NASM** 把汇编译成与 HxD 手工版 **同一映像**
- 引导扇区 **512 B**，末尾 **`55 AA`**；汇编 **`$`** 自动 padding
- **IPL** · **Boot（bootstrap）**

→ 详读 [day-01-boot-asm/](./day-01-boot-asm/)

---

## Day 2 要点速览

- **`ORG` / `JMP` / `MOV`** · 16 位寄存器 · **`[]` 访存**（慢于寄存器）
- **`INT` → BIOS** 做早期 I/O；引导扇区加载到 **`0x7c00`**
- 只编 **`ipl.bin`（512 B）**，整盘用映像工具拼
- **`Makefile` + `make`** 替代一堆 `.bat`

→ 详读 [day-02-asm-makefile/](./day-02-asm-makefile/)

---

## Day 3 要点速览

- 真正 **IPL**：**`INT 0x13`** 读盘，**10 柱面 / 180KB**，**重试 5 次**
- **haribote-os** · **`INT 0x10` → 320×200×8** 全黑 = OS 运行
- **32 位前** 做完 BIOS；**`bootpack.c` / `HariMain`**
- **`asmfunc.asm`**（原书 `naskfunc.asm`）· **`io_hlt`** — 汇编 + C 链接

→ 详读 [day-03-32bit-c/](./day-03-32bit-c/)

---

## Day 4 要点速览

- **VRAM `0xa0000`** · **`for` + 位运算** → 白屏 / 条纹
- **`char *p` / `*p`** = 字节宽内存写
- **调色板** · 端口 **`0x03c8`/`0x03c9`** · **`OUT`**
- **`CLI`/`STI`** · **`PUSHFD`/`POPFD`**（EFLAGS）
- **画矩形** → 底部任务条 · 内核 ~**1.2KB**

→ 详读 [day-04-c-graphics/](./day-04-c-graphics/)

---

## Day 5 要点速览

- **`BOOTINFO` struct** · **`->`** 动态读启动画面信息
- **8×16 点阵** · 外部字体 · **`sprintf`** 屏幕调试
- **16×16 鼠标箭头** · 透明背景
- **GDT** 分段 · **IDT** 0~255 中断 → 处理函数

→ 详读 [day-05-gdt-idt/](./day-05-gdt-idt/)

---

## Day 6 要点速览

- 拆 **`graphic.c` / `dsctbl.c`** · Makefile **模式规则**
- **`LGDT`** · **Ring0 / Ring3** 段属性
- **PIC** 主/从级联 · 端口初始化
- **`_asm_inthandler` + C + `IRETD`** · **栈 LIFO**
- **IRQ1→0x21** 键盘 · 按 **A** 屏幕有提示

→ 详读 [day-06-split-compile-irq/](./day-06-split-compile-irq/)

---

## Day 7 要点速览

- 端口 **`0x0060`** · 扫描码 **1E/9E** · ISR **只入队**
- **环形 FIFO** · **`FIFO8`**（替代移位版）
- 鼠标经 **键盘控制器** **`0xFA`** · **IRQ12** · **128B FIFO**
- 晃动鼠标 → **hex 瀑布**（指针 Day 8）

→ 详读 [day-07-fifo-mouse/](./day-07-fifo-mouse/)

---

## Day 8 要点速览

- **`MOUSE_DEC`** · 3 字节包 · **`&0xC8==0x08`** 同步
- **mx/my** · 边界 · 擦旧画新 · 任务栏 **无图层** 伏笔
- **A20GATE** · **CR0** · **bootpack @ 0x00280000** · **far-JMP → C**
- Day 3–8 **硬件初始化** 贯通

→ 详读 [day-08-mouse-32bit/](./day-08-mouse-32bit/)

---

## Day 9 要点速览

- 探针 **0xaa55aa55** · **CR0 关 Cache** 再测容量
- C memtest 被 **优化删循环** → **汇编 memtest_sub**
- 位图 3GB 要 **~768KB** 元数据 → **MEMMAN 空闲块列表**
- **alloc / free** · **合并相邻空闲块**

→ 详读 [day-09-memory/](./day-09-memory/)

---

## Day 10 要点速览

- **4KB 对齐** · **AND 舍入**（MEMMAN 续）
- **SHEET / SHTCTL** · Z 序：背景 → 窗口 → 鼠标
- **局部 refresh** · ~**0.8%** 像素 vs 全屏 64000
- **倒推 bx0/bx1** · 循环只跑脏区

→ 详读 [day-10-layers/](./day-10-layers/)

---

## Day 11 要点速览

- **`sheet_refreshsub` 屏界裁切** — 出屏不乱画
- **`make_window8`** · **SHEET→SHTCTL** 指针
- 高速计数器 · **只刷变化层及以上**
- **`map[]` 像素归属** — 下层写 VRAM 前 **skip 上层像素**

→ 详读 [day-11-window/](./day-11-window/)

---

## Day 12 要点速览

- **PIT** · **IRQ0** · **`inthandler20`** · ~**100Hz**
- **`TIMERCTL.count`** — OS 秒表
- **timeout → FIFO** · **500 路** · **`TIMER_FLAGS_USING`**
- **`next`** — 只处理最近到期，避免每 tick 扫 500

→ 详读 [day-12-timer1/](./day-12-timer1/)

---

## Day 13 要点速览

- **统一 32 位 FIFO** · 256 键盘 / 512 鼠标 / 小值定时器
- 计数器 **3s 后** 测 **10s max**
- 定时器 **链表 next** — 无数组移位
- **哨兵 `0xFFFFFFFF`** — 简化插入

→ 详读 [day-13-timer2/](./day-13-timer2/)

---

## Day 14 要点速览

- Day 13 优化 → **中断禁止时间↓**
- **VBE 0x4F02** · **640×480** · 失败 **320×200**
- **`keytable[]`** · Backspace · 闪烁光标
- **`sheet_slide`** · 左键 **拖窗口**

→ 详读 [day-14-keyboard/](./day-14-keyboard/)

---

## Day 15 要点速览

- 单核 **时间片 ~0.01–0.03s** · **TSS/TR** · **far-JMP** 切换
- **0.02s** 定时器 **A↔B** · task B **窗口计数**
- **TSS 初始栈 [ESP+4]** 传 **`sht_back`**
- **`mt_taskswitch` 进 timer ISR** — 抢占式调度雏形

→ 详读 [day-15-multitask1/](./day-15-multitask1/)

---

## Day 16 要点速览

- **`TASKCTL`** · **`task_alloc` / `task_run`**
- **`task_sleep` / FIFO wake**
- **A + B0/B1/B2** 四任务
- **`TASKLEVEL`** · 高层未睡 **不跑低层** · UI **瞬间抢占**

→ 详读 [day-16-multitask2/](./day-16-multitask2/)

---

## Day 17 要点速览

- **`console_task`** · **Idle** 最低优先级
- **Tab / `key_to`** · 标题栏焦点色
- **TASK.fifo** · Backspace **= 8**
- **`keytable0/1`** · CapsLock · **±0x20**
- **LED `0xED` @ 0x60**

→ 详读 [day-17-console/](./day-17-console/)

---

## Day 18 要点速览

- FIFO **2/3** 控制 **焦点光标** 闪/停
- Enter **10** · **`cons_newline`** 滚屏 **+16px**
- **`strcmp`** · **mem / cls**
- **FAT12** 根目录 **224×32B** · **`dir`**

→ 详读 [day-18-dir/](./day-18-dir/)

---

## Day 19 要点速览

- **`type`** · **clustno×512+0x003e00**
- **\\n \\r \\t** 排版
- **FAT12 解压** · 链到 **0xFFF**
- **window/console/file.c**
- **`hlt.hrb`** · **GDT + farjmp**

→ 详读 [day-19-apps/](./day-19-apps/)

---

## Day 20 要点速览

- **far-CALL / RETF** 回 Shell
- **INT 0x40** 稳定 API · 替代硬编码地址
- **`cmd_app`** → 任意 **`.hrb`**
- **PUSHAD/POPAD** 保寄存器
- **EDX** 功能号 **1/2/3** 路由

→ 详读 [day-20-api/](./day-20-api/)

---

## Day 21 要点速览

- app 数据基址 **`0xfe8`** · 字符串 API 修 DS
- **C app** · **`_api_*`** · **CALL 0x1b + RETF**
- **1003/1004** 段 · **`crack1.hrb`** 防御
- **INT 0x0d** · **DPL +0x60** · 仅 **INT 0x40** 出入口

→ 详读 [day-21-protection/](./day-21-protection/)

---

## Day 22 要点速览

- **CLI/HLT/IN/OUT → 0x0d**
- **0x0c** · **EIP + .map/.lst** · **Shift+F1** 强杀
- **.hrb 36B Hari 头** · 数据段装载
- **窗口句柄** · 字符/方块 **GUI API**

→ 详读 [day-22-c-apps/](./day-22-c-apps/)

---

## Day 23 要点速览

- **EDX 9/10** malloc · **7.6KB→387B**
- **11/12** 画点 · **LSB 禁 auto-refresh**
- **13** 画线 · **1024 定点 >>10**
- **15** 键盘 · **walk.hrb**
- **14** + **sheet→task** 强杀清窗

→ 详读 [day-23-graphics/](./day-23-graphics/)

---

## Day 24 要点速览

- **F11 / Tab / key_win** · **点击激活** 置顶+焦点
- 标题栏 **拖拽** · **× 关 app**
- **EDX 16–19** · **noodle.hrb**
- app 退出 **清遗留 timer** — 防 Console 乱码

→ 详读 [day-24-window-ops/](./day-24-window-ops/)

---

## Day 25 要点速览

- **PIT 蜂鸣器** · **216 色 + 抖动**
- **`cons[]`** · **TASK.cons / ds_base**
- **sel → 代码 1003~2002 / 数据 2003~3002**
- 删 **task_a** · **key_win==0** 判空

→ 详读 [day-25-multi-console/](./day-25-multi-console/)

---

## Day 26 要点速览

- **32 位写** refresh · **~4×** 绘图
- **new_mx/my** · **FIFO 空才刷新**
- 开机 **1 Console** · **Shift+F2** 新开 · **× 可关**
- **`start`** · **`ncst`** 无 Console GUI

→ 详读 [day-26-window-speed/](./day-26-window-speed/)

---

## Day 27 要点速览

- **ncst** 关窗 · **先 hide 后 free**
- **LDT** 防 **crack7** · app 互不可见段
- API **按 .obj 链接** · **`apilib.lib`**
- **haribote/apilib** · **`app_make.txt`**

→ 详读 [day-27-ldt-lib/](./day-27-ldt-lib/)

---

## Day 28 要点速览

- **`__alloca`** · **winhelo 7664→174B**
- 文件 **EDX 21–25** · 命令行 **26** · langmode **27**
- **`type.hrb`** 替代内核 type
- **`nihongo.fnt`** · **16×16 全角**

→ 详读 [day-28-files/](./day-28-files/)

---

## Day 29 要点速览

- 全角 refresh **`x-8`**
- **tek / loadfile2** · **nihongo 142→56.6KB**
- **printf/malloc** 标准层
- **透明 255** · **bball**
- **Invader ~2.28KB**

→ 详读 [day-29-compression/](./day-29-compression/)

---

## Day 30 要点速览 · 收官

- **calc.hrb** · **tview.hrb** · **mmlplay.hrb** · **gview.hrb**
- IPL **多扇区读** · **9 柱面** · tek 瘦身
- **全书 30 天笔记完成**

→ 详读 [day-30-advanced-apps/](./day-30-advanced-apps/)
