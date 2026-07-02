# §3.4 · Bootpack 四文件：汇编 + C 完整分工

Day 3 的 OS 本体 **bootpack** 由 **4 类源文件** 链接而成（IPL 单独占软盘第 1 扇区）。本目录含其中 **3 个**；第 4 个 **`ipl.asm`** 在 [sec-3.1-ipl-int13-disk-load](../sec-3.1-ipl-int13-disk-load/)。

笔记：[§3.4.2 四文件分工](../../notes/section-3.4.2-io_hlt与工程分层.md) · 极简 GDT 示例：[sec-3.4-minimal-16-to-32-call-c](../sec-3.4-minimal-16-to-32-call-c/)

---

## 总览

| 文件 | 路径 | CPU 模式 | 何时跑 | 一句话职责 |
|------|------|----------|--------|------------|
| **`ipl.asm`** | [../sec-3.1-ipl-int13-disk-load/ipl.asm](../sec-3.1-ipl-int13-disk-load/ipl.asm) | 16 位实模式 | **最先** | 读 bootpack 进内存，`JMP 0x8200` |
| **`nasmhead.asm`** | [./nasmhead.asm](./nasmhead.asm) | 16→32 | IPL 跳进来后 | VGA、A20、GDT、CR0，**`call HariMain`** |
| **`bootpack.c`** | [./bootpack.c](./bootpack.c) | 32 位 C | nasmhead 调用后 | **`HariMain`** — OS 主逻辑 |
| **`asmfunc.asm`** | [./asmfunc.asm](./asmfunc.asm) | 32 位 asm | C `call` 时 | **`io_hlt`** 等 C 写不了的指令 |

---

## 1. `ipl.asm` — 搬运工（§3.1 目录）

| 项目 | 内容 |
|------|------|
| **路径** | [../sec-3.1-ipl-int13-disk-load/ipl.asm](../sec-3.1-ipl-int13-disk-load/ipl.asm) |
| **体积** | **512 B** 上限（一个引导扇区） |
| **何时跑** | BIOS 加载软盘扇区 1 → `0x7C00` |
| **干什么** | FAT12 头 + **`INT 0x13`** 循环读 **10 柱面** bootpack 到 **`0x8200`**；`load done`；**`JMP 0x8200`** |
| **不干什么** | 不切保护模式、不跑 C、不切 VGA |
| **编译** | `nasm -f bin ipl.asm -o ipl.bin` → 贴软盘 **偏移 0** |

---

## 2. `nasmhead.asm` — 搭台（本目录 ✅）

| 项目 | 内容 |
|------|------|
| **路径** | [./nasmhead.asm](./nasmhead.asm) |
| **何时跑** | CPU 执行 **`JMP 0x8200`** 后，从标签 **`start`** 开始 |
| **干什么** | 见下表 |
| **编译** | `nasm -f elf32 nasmhead.asm -o nasmhead.o` |

### 代码块分工

| 步骤 | 模式 | 代码 | 作用 |
|------|------|------|------|
| 关中断 | 16 位 | `cli` | 切模式期间不被中断打断 |
| 切 VGA | 16 位 | `mov ax,0x0013` + `int 0x10` | §3.2：**320×200×256** 图形模式（必须 16 位调 BIOS） |
| 开 A20 | 16 位 | `in/out 0x92` | 地址线 A20，否则保护模式下访问高内存异常 |
| 进保护模式 | 16 位 | `lgdt` + 置 `CR0.PE` + 远跳转 | 建立 GDT，打开 **CR0 第 0 位** |
| 设段与栈 | 32 位 | `init_pm`：`mov ds/es/...`、`esp=0x90000` | 32 位平坦段 + 栈就绪 |
| 交 C | 32 位 | **`call HariMain`** | ★ asm 搭台完毕，OS 逻辑交给 C |
| 兜底 | 32 位 | `hang`：`hlt` 循环 | C 返回则停住 |

---

## 3. `bootpack.c` — 唱戏（本目录 ✅）

| 项目 | 内容 |
|------|------|
| **路径** | [./bootpack.c](./bootpack.c) |
| **入口** | **`void HariMain(void)`** — 原书 OS 主入口 |
| **何时跑** | nasmhead **`call HariMain`** 之后 |
| **CPU 模式** | **32 位保护模式** |
| **干什么** | 把 **`0xA0000`** 起 **320×200** 字节填 **0**（调色板 0 = 黑）→ §3.2 **黑屏验收**；然后 **`for(;;) io_hlt();`** |
| **依赖** | 声明 **`void io_hlt(void);`** — 实现在 asmfunc |
| **编译** | `gcc -m32 -c bootpack.c -o bootpack.o -ffreestanding -nostdlib -fno-pie` |

C 适合写：循环、数据结构、业务逻辑。  
C 不适合写：切 `CR0`、远跳转、`HLT` — 这些在 asm 里。

---

## 4. `asmfunc.asm` — 补丁（本目录 ✅）

| 项目 | 内容 |
|------|------|
| **路径** | [./asmfunc.asm](./asmfunc.asm) |
| **何时跑** | bootpack.c 里 **`io_hlt();`** 被调用时 |
| **CPU 模式** | **32 位**（与 C 一致） |
| **干什么** | **`io_hlt`**：`HLT`（CPU 休眠）→ **`RET`**（回到 C 的下一条指令） |
| **为何独立文件** | 小函数、纯指令；和 nasmhead 几百行启动代码分开 |
| **编译** | `nasm -f elf32 asmfunc.asm -o asmfunc.o` |

链接器负责：把 C 里的 **`call io_hlt`** 机器码指到 asm 里的 **`io_hlt`** 标签。

---

## 两种「汇编 + C」配合

| 方向 | 谁 → 谁 | 交接方式 | 本仓库例子 |
|------|---------|----------|------------|
| **大块 asm 末尾 call C** | nasmhead → bootpack.c | 切完 32 位、栈好 → **`call HariMain`** | [nasmhead.asm](./nasmhead.asm) + [bootpack.c](./bootpack.c) |
| **C 里 call 小 asm** | bootpack.c → asmfunc | 普通函数调用 → asm **`RET` 回 C** | [bootpack.c](./bootpack.c) + [asmfunc.asm](./asmfunc.asm) |

---

## 运行时顺序

```text
[ 磁盘 ]
  ipl.bin           @ 偏移 0    ← 仅 ipl.asm
  bootpack 二进制   @ 偏移 512  ← nasmhead + bootpack.c + asmfunc 链接结果

[ CPU ]
  ① BIOS → ipl.asm（16 位）→ INT 0x13 读 bootpack → JMP 0x8200
  ② nasmhead.asm（16→32）→ INT 0x10 VGA、A20、GDT、CR0 → call HariMain
  ③ bootpack.c（32 位）→ HariMain 填黑屏 → io_hlt()
  ④ asmfunc.asm（32 位）→ HLT; RET → 回到 ③ 的循环
```

---

## 链接 bootpack（Linux / WSL / MSYS2）

链接地址必须等于 IPL 跳转目标 **`0x8200`**。

```bash
cd sec-3.4-bootpack-asm-and-c

nasm -f elf32 nasmhead.asm -o nasmhead.o
nasm -f elf32 asmfunc.asm   -o asmfunc.o
gcc -m32 -c bootpack.c -o bootpack.o -ffreestanding -nostdlib -fno-pie

ld -m elf_i386 -Ttext 0x8200 -e start -o bootpack.bin \
   nasmhead.o bootpack.o asmfunc.o --oformat binary
```

| 步骤 | 输入 | 输出 |
|------|------|------|
| 汇编 nasmhead / asmfunc | `.asm` | `.o` |
| 编译 bootpack.c | `.c` | `.o` |
| 链接 | 三个 `.o` | **`bootpack.bin`**（写入软盘扇区 2 起） |

拼软盘：把 [ipl.bin](../sec-3.1-ipl-int13-disk-load/ipl.bin) 写偏移 0，`bootpack.bin` 写偏移 512 — 见 [§3.1 README](../sec-3.1-ipl-int13-disk-load/README.md)，再用 QEMU 启动。

> 教学简化版；原书 haribote Makefile 后续还会加更多 `.o` 与精确扇区布局。

---

## 自检

- [ ] 四个文件名、各自 **16 位还是 32 位** 能说清楚  
- [ ] 知道 **`call HariMain`** 和 **`io_hlt()`** 两种交接方向  
- [ ] 能打开四个源文件，各找到 **入口标签 / 主函数 / HLT**  
- [ ] 对照极简版：[sec-3.4-minimal-16-to-32-call-c](../sec-3.4-minimal-16-to-32-call-c/)
