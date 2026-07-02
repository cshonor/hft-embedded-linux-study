# §3.4.3 · 极简示例：汇编搭台 → C 唱戏

本目录 **不是** 完整可启动软盘，只演示一件事：**nasmhead 切到 32 位保护模式后，`call` C 写的入口函数**。

完整四文件 bootpack（含 IPL 读盘、VGA、`io_hlt`）：[sec-3.4-bootpack-asm-and-c](../sec-3.4-bootpack-asm-and-c/)  
笔记：[§3.4.3](../../notes/section-3.4.3-16切32与call-C完整例子.md) · [§3.4.2 四文件分工](../../notes/section-3.4.2-io_hlt与工程分层.md)

---

## 本目录里有什么

| 文件 | 在本示例中？ | 说明 |
|------|-------------|------|
| [nasmhead-minimal.asm](./nasmhead-minimal.asm) | ✅ 有 | 16→32 切换 + `call kernel_main` |
| [kernel_main.c](./kernel_main.c) | ✅ 有 | 32 位 C 入口，写 VGA 文本 |
| `ipl.asm` | ❌ 未含 | 在 [sec-3.1-ipl-int13-disk-load](../sec-3.1-ipl-int13-disk-load/) |
| `asmfunc.asm` | ❌ 未含 | 在 [sec-3.4-bootpack-asm-and-c](../sec-3.4-bootpack-asm-and-c/) |

下面按 **Day 3 启动链上的四个文件** 逐个说明；标「本目录」的是你能直接打开的源码。

---

## 1. `ipl.asm` — 搬运工（不在本目录）

| 项目 | 内容 |
|------|------|
| **路径** | [../sec-3.1-ipl-int13-disk-load/ipl.asm](../sec-3.1-ipl-int13-disk-load/ipl.asm) |
| **何时跑** | 最先 — BIOS 把软盘 **第 1 扇区（512 B）** 读到 `0x7C00` 后 |
| **CPU 模式** | **16 位实模式** |
| **干什么** | 用 **`INT 0x13`** 从扇区 2 起读 **bootpack** 到内存 **`0x8200`**；打印 `load done`；**`JMP 0x8200`** 跳进 bootpack |
| **不干什么** | 不切 32 位、不调 C、不切 VGA（512 B 装不下） |
| **产物** | 单独 `nasm -f bin` → **`ipl.bin`**，贴软盘偏移 0 |

本极简示例 **省略 IPL**：假设 bootpack 已经在内存里，直接从 nasmhead 入口开始看。

---

## 2. `nasmhead-minimal.asm` — 搭台（本目录 ✅）

| 项目 | 内容 |
|------|------|
| **路径** | [./nasmhead-minimal.asm](./nasmhead-minimal.asm) |
| **何时跑** | IPL `JMP 0x8200` 之后（本示例里你手动从 `switch_to_32` 想象这条链） |
| **CPU 模式** | **先 16 位，再切 32 位保护模式** |
| **干什么** | 见下表「代码块分工」 |
| **不干什么（相对原书）** | 无 **`INT 0x10` 切 VGA**、无 **开 A20** — 为缩短代码，只保留 GDT + CR0 |
| **产物** | `nasm -f elf32` → **`nasmhead.o`**，链接进 bootpack |

### 代码块分工

| 符号 / 区域 | 模式 | 作用 |
|-------------|------|------|
| **`switch_to_32`** | 16 位 | 入口标签；`cli` 关中断 |
| **`lgdt [gdt_desc]`** | 16 位 | 加载全局描述符表 |
| **写 `CR0.PE = 1`** | 16 位 | 打开保护模式位 |
| **`jmp CODE_SEL:init_pm`** | 16→32 | 远跳转，刷新流水线，进入 32 位代码段 |
| **`init_pm`** | 32 位 | 给 DS/ES/FS/GS/SS 赋数据段选择子；**`esp = 0x90000`** 设栈 |
| **`call kernel_main`** | 32 位 | ★ **舞台搭好，把控制权交给 C** |
| **`hang`** | 32 位 | C 若返回则 `hlt` 死循环 |
| **`gdt` / `gdt_desc`** | 16 位数据 | 空描述符 + 平坦 4G 代码段 + 平坦 4G 数据段 |

原书完整版：[../sec-3.4-bootpack-asm-and-c/nasmhead.asm](../sec-3.4-bootpack-asm-and-c/nasmhead.asm)（多 VGA、A20，入口叫 **`start`**，调用 **`HariMain`**）。

---

## 3. `kernel_main.c` — 唱戏（本目录 ✅）

| 项目 | 内容 |
|------|------|
| **路径** | [./kernel_main.c](./kernel_main.c) |
| **何时跑** | nasmhead **`call kernel_main`** 之后 |
| **CPU 模式** | **32 位保护模式**（必须 — 若还在 16 位实模式，这段 C 的机器码不能跑） |
| **干什么** | **`kernel_main()`**：用 `for` 循环把 `"Hello 32-bit C!"` 写到 **VGA 文本缓冲 `0xB8000`**（每字符 2 字节：字 + 属性 `0x07`）；然后空转 `for(;;)` |
| **不干什么** | 不切换 CPU 模式、不调 BIOS（32 位下也调不了） |
| **产物** | `gcc -m32 -c` → **`kernel_main.o`** |

原书对应：**`bootpack.c`** 里的 **`HariMain()`** — 见 [../sec-3.4-bootpack-asm-and-c/bootpack.c](../sec-3.4-bootpack-asm-and-c/bootpack.c)（写 **`0xA0000`** 图形缓冲填黑 + 调 **`io_hlt()`**）。

> 本示例用 **`kernel_main`** 这个名字，是为了和通用教程对齐；链接时 nasmhead 里 `extern kernel_main` 必须和 C 函数名一致。

---

## 4. `asmfunc.asm` — 补丁（不在本目录）

| 项目 | 内容 |
|------|------|
| **路径** | [../sec-3.4-bootpack-asm-and-c/asmfunc.asm](../sec-3.4-bootpack-asm-and-c/asmfunc.asm) |
| **何时跑** | **被 C `call`** 时（例如 `HariMain` 里 `io_hlt();`） |
| **CPU 模式** | **32 位**（与 C 同模式） |
| **干什么** | **`io_hlt`**：`HLT` 让 CPU 休眠，再 **`RET`** 回到 C |
| **为何单独文件** | 几条指令的小函数；与 nasmhead **大块启动代码** 分开，结构清晰 |
| **产物** | `nasm -f elf32` → **`asmfunc.o`** |

本极简示例 **省略 asmfunc**：`kernel_main` 末尾只是空 `for(;;)`，没有 `io_hlt()`。

---

## 运行时顺序（含原书完整链）

```text
[ 完整 haribote 软盘 ]
  ① ipl.asm（16 位）     INT 0x13 读盘 → JMP 0x8200
  ② nasmhead.asm（16→32） INT 0x10 VGA、A20、GDT、CR0 → call HariMain
  ③ bootpack.c（32 位 C） HariMain：业务逻辑
  ④ asmfunc.asm（32 位）  C 调用 io_hlt() → HLT; RET

[ 本目录极简示例 — 只看 ②③ 的缩小版 ]
  switch_to_32（16 位 asm）→ init_pm（32 位 asm）→ call kernel_main（C）→ 写 0xB8000
```

---

## 编译与链接（Linux / WSL / MSYS2）

需要：**nasm**、**gcc 32 位**、**ld**。

```bash
cd sec-3.4-minimal-16-to-32-call-c

# 汇编 → 目标文件
nasm -f elf32 nasmhead-minimal.asm -o nasmhead.o
gcc -m32 -c kernel_main.c -o kernel_main.o -ffreestanding -nostdlib -fno-pie

# 链接 → 一个 ELF（本示例不绑 0x8200，只验证 asm/C 能连上）
ld -m elf_i386 -o bootpack.elf nasmhead.o kernel_main.o
```

| 步骤 | 输入 | 输出 | 说明 |
|------|------|------|------|
| `nasm -f elf32` | `nasmhead-minimal.asm` | `nasmhead.o` | 含 GDT、切模式、`call kernel_main` |
| `gcc -m32 -c` | `kernel_main.c` | `kernel_main.o` | 32 位 C 机器码 |
| `ld` | 两个 `.o` | `bootpack.elf` | 链接器把 `call kernel_main` 连到 C 入口 |

> 真正写进软盘还要 **`-Ttext 0x8200`** 与 IPL 跳转地址对齐，并和 **ipl.bin** 拼镜像 — 见 [sec-3.4-bootpack-asm-and-c/README.md](../sec-3.4-bootpack-asm-and-c/README.md)。

---

## 与原书 haribote 对照

| 本目录 | 原书 Day 3 | 差异 |
|--------|------------|------|
| `nasmhead-minimal.asm` | `nasmhead.asm` | 无 VGA、无 A20；入口 `switch_to_32` vs `start` |
| `kernel_main()` | **`HariMain()`** | 写文本 `0xB8000` vs 写图形 `0xA0000` 填黑 |
| （未含） | `asmfunc.asm` | 无 **`io_hlt()`** |
| （未含） | `ipl.asm` | 无读盘、无软盘布局 |

---

## 自检

- [ ] 能说出 **本目录两个文件** 各自在什么 CPU 模式下跑  
- [ ] 能指出 **`call kernel_main`** 在源码哪一行、谁调用谁  
- [ ] 知道 **ipl / asmfunc** 在哪个文件夹、本示例为何省略它们  
- [ ] 对照 [sec-3.4-bootpack-asm-and-c](../sec-3.4-bootpack-asm-and-c/) 看完整四文件版
