## ④.2 `io_hlt` 与工程分层

Day 3 起，OS 本体 **bootpack** 不是单个文件，而是 **4 类源文件链接在一起**。记清 **谁、何时、干什么**，后面读 Makefile 才不晕。

---

### 四文件分工（总表）

| 文件 | 后缀 | 何时跑 | CPU 模式 | 职责 |
|------|------|--------|----------|------|
| **`ipl.asm`** | `.asm` | **最先** — BIOS 加载第 1 扇区 | **16 位实模式** | **512 B 引导**：`INT 0x13` 读 bootpack → `JMP 0x8200`；**尚无 C** |
| **`nasmhead.asm`** | `.asm` | bootpack **入口** — IPL 跳进来后 | **先 16 位，再切 32 位** | **切 VGA**（`INT 0x10`）、**开 A20**、**建 GDT**、**CR0 进保护模式**；最后 **`call HariMain`** |
| **`bootpack.c`** | `.c` | nasmhead **`call HariMain`** 之后 | **32 位保护模式** | **`HariMain`** — OS **主逻辑**（图形、内存、键鼠…）；**可以调** `io_hlt()` |
| **`asmfunc.asm`** | `.asm` | 被 C **`call`** 时 | **32 位**（与 C 同模式） | **`io_hlt`** 等 — C **写不了的指令**（`HLT`…）；几条 asm + `RET` |

**一句话：**

- **`ipl.asm`** — 软盘上 **512 B**，单独占 **第 1 扇区**（[sec-3.1-ipl-int13-disk-load/ipl.asm](../code/sec-3.1-ipl-int13-disk-load/ipl.asm)）  
- **`nasmhead` + `bootpack.c` + `asmfunc`** — [sec-3.4-bootpack-asm-and-c/](../code/sec-3.4-bootpack-asm-and-c/)，被 IPL **一次性读进 `0x8200` 起** 的内存  

---

### 四个文件怎么配合？（执行顺序）

```text
[ 磁盘 ]
  ipl.bin          ← 仅 ipl.asm（512 B，扇区 1）
  bootpack 区      ← nasmhead + bootpack.c + asmfunc 链接后的二进制（扇区 2 起）

[ 运行顺序 ]
  ① BIOS 读 ipl.bin → 0x7C00，跑 ipl.asm（16 位）
         INT 0x13 读 bootpack 整包 → 0x8200
         JMP 0x8200
  ② 从 0x8200 进入 nasmhead.asm（16 位）
         INT 0x10 切 VGA、开 A20、LGDT、CR0…
         切到 32 位
         call HariMain          ← asm 交给 C
  ③ bootpack.c 的 HariMain（32 位 C）
         业务逻辑…
         io_hlt();              ← C 调 asmfunc
  ④ asmfunc.asm 的 io_hlt（32 位 asm）
         HLT; RET
```

**链接阶段（编译时，不是运行时）：**

```text
ipl.asm          ──nasm -f bin──►  ipl.bin（单独 512 B，贴软盘偏移 0）

nasmhead.asm  ┐
bootpack.c    ├── 各自编成 .o ──ld 链接──►  bootpack 二进制（写入软盘扇区 2 起）
asmfunc.asm   ┘
```

| 阶段 | 谁干 | 结果 |
|------|------|------|
| **汇编 ipl** | `nasm -f bin ipl.asm` | **512 B** `ipl.bin` |
| **汇编 nasmhead / asmfunc** | `nasm -f elf32 …` | `.o` |
| **编译 C** | `gcc -m32 -c bootpack.c` | `.o` |
| **链接 bootpack** | `ld …` | 一个 **bootpack** 可执行块 |
| **写盘** | 工具 / `dd` | `ipl.bin` @ 0 + bootpack @ 512 |

---

### 逐个文件写什么（对照表）

#### 1. `ipl.asm` — 搬运工（16 位，512 B 上限）

| 项目 | 说明 |
|------|------|
| **干什么** | FAT12 头 + **`INT 0x13` 循环读盘** + `load done` + **`JMP 0x8200`** |
| **不干什么** | 不切 32 位、不跑 C、不切 VGA（装不下） |
| **仓库** | [ipl.asm](../code/sec-3.1-ipl-int13-disk-load/ipl.asm) ✅ |

#### 2. `nasmhead.asm` — 搭台（16→32，大块 asm）

| 项目 | 说明 |
|------|------|
| **干什么** | BIOS 能做的事在这里收尾（**`INT 0x10` 图形模式**）；然后 **A20、GDT、CR0**，**切 32 位**，设 **ESP**，**`call HariMain`** |
| **为何不写 C** | 切模式、写 **CR0**、远跳转 — C 没有对应语法 |
| **对照** | [nasmhead.asm](../code/sec-3.4-bootpack-asm-and-c/nasmhead.asm) · [§3.4.3 极简版](../code/sec-3.4-minimal-16-to-32-call-c/nasmhead-minimal.asm) |

#### 3. `bootpack.c` — 唱戏（32 位 C 主逻辑）

| 项目 | 说明 |
|------|------|
| **干什么** | **`void HariMain(void)`** — 原书 OS 入口；循环、数据结构、调 API |
| **可以调用** | `io_hlt()` 等声明在 C、实现在 **asmfunc** 的函数 |
| **仓库** | [bootpack.c](../code/sec-3.4-bootpack-asm-and-c/bootpack.c) ✅ |
| **编译** | `gcc -m32` — 生成 **32 位** 机器码，必须在 nasmhead **切完模式之后** 才跑 |

#### 4. `asmfunc.asm` — 补丁（32 位，给 C 用的几条指令）

| 项目 | 说明 |
|------|------|
| **干什么** | **`io_hlt`：`HLT` + `RET`** — 让 CPU 休眠省电 |
| **为何单独文件** | 小函数、纯指令；与 nasmhead **大块启动** 分开，清晰 |
| **C 侧** | `void io_hlt(void);` 声明 → `io_hlt();` 调用 → 链接器连到 asm 标签 |
| **仓库** | [asmfunc.asm](../code/sec-3.4-bootpack-asm-and-c/asmfunc.asm) ✅ |

---

### 两种「汇编 + C」配合方式

| 方式 | 文件 | 交接 | 例子 |
|------|------|------|------|
| **大块 asm 末尾 `call` C** | **nasmhead → bootpack.c** | 切完 32 位、栈就绪 → **`call HariMain`** | 搭台 → 唱戏 |
| **C 里 `call` 小 asm 函数** | **bootpack.c → asmfunc** | C 当普通函数调 → asm **`RET` 回 C** | `io_hlt()` |

---

### 典型例子：`io_hlt`

**asmfunc.asm（示意）：**

```nasm
        BITS 32
        GLOBAL io_hlt

io_hlt:
        HLT
        RET
```

**bootpack.c（示意）：**

```c
void io_hlt(void);

void HariMain(void) {
    for (;;) {
        io_hlt();
    }
}
```

| 谁 | 干什么 |
|----|--------|
| **C** | 决定 **什么时候** 休眠 |
| **汇编** | 执行 **HLT** |
| **链接器** | 把 `call io_hlt` 连到 asm 入口 |

更大块：**nasmhead** 里 **GDT + CR0** 整段留在 asm，切完后 **`call HariMain`** — 见 [§3.4.3](./section-3.4.3-16切32与call-C完整例子.md)。

---

← [§3.4.1 为何需要 asm](./section-3.4.1-为何C需要汇编包装.md) · [§3.4.3 完整例子 →](./section-3.4.3-16切32与call-C完整例子.md)
