# 工具链选型 · NASM + GCC + Make

> **本仓库立场：** 全程用 **原版 NASM**，不用原书作者魔改的 **nask**；C 侧用 **GCC**（替代 tolset 里的 **bcc**），构建用 **GNU Make + QEMU**。

---

## 一句话

| | 原书（tolset） | **本仓库** |
|---|----------------|------------|
| 汇编器 | **nask**（作者基于 NASM 风格魔改） | **NASM** |
| C 编译器 | **bcc** | **GCC**（MinGW-w64 / MSYS2） |
| 构建 | 书内 Makefile + 批处理 | **GNU Make** |
| 运行 | QEMU / 软驱 | **QEMU** |

**nask 可以这么理解：** 川合秀实为了方便读者，在 NASM 语法风格上做了定制汇编器。**我们直接用原版 NASM** — 源码后缀统一 **`.asm`**（原书 `.nas`）。

---

## 汇编器：nask → NASM，源码统一 `.asm`

原书 tolset 里是 **nask**；本仓库用 **NASM**。**换的是参与编译的软件；源码后缀从原书 `.nas` 统一为 `.asm`。**

| 什么 **不用改** | 什么 **要换** |
|----------------|--------------|
| 源码仍叫 **`helloos.asm`**（对应原书 `helloos.nas`） | 汇编器：**nask** → **`nasm.exe`** |
| 后缀统一 **`.asm`** | 编译 **命令格式**（见下表） |
| 汇编 **语法** 多数逐行相同 | 必须加 **`-f bin`**（见下） |

```
原书：  helloos.nas  ──nask────────►  helloos.img / ipl.bin
本仓库：helloos.asm  ──nasm -f bin──►  helloos.img / ipl.bin
```

| 原书（nask） | **本仓库（NASM）** |
|--------------|-------------------|
| `nask helloos.nas helloos.img` | **`nasm -f bin helloos.asm -o helloos.img`** |
| `nask helloos.nas helloos.lst ipl.bin` | **`nasm -f bin helloos.asm -o ipl.bin -l helloos.lst`** |

**`-f bin` 是关键：** 告诉 NASM **不要** 生成带格式的目标文件（如 `.obj`），而是 **直接输出纯二进制** — 与 nask 默认行为一致，才能作为 **512 B 启动区**，再拼进映像、用 **QEMU** 启动。

→ 深入理解 **`.bin` / `-f bin` vs `-f elf` / `.img`**：见下一节 **[`.bin` 是什么？](#bin-是什么-f-bin-vs-f-elf-vs-img)**。

> **不必** 为了用 NASM 而把 `helloos.asm` 改成 `boot.asm` — 笔记里若出现 `boot.asm` / `ipl.asm`，只是 **通称**；**本仓库默认文件名仍是 `helloos.asm`**。

---

## `.bin` 是什么？`-f bin` vs `-f elf` vs `.img`

**结论：`.bin` 泛指原始裸二进制文件。**

### 1、`.bin` 的核心特质

| | `.bin`（裸二进制） | `.o` / ELF 等带格式目标文件 |
|---|-------------------|------------------------------|
| **头部** | **无** — 没有格式化头、段描述符、重定位表、符号表 | **有** — 程序头、段信息、链接元数据 |
| **文件里每一字节** | **直接就是** CPU 逐条执行的 **原生机器指令 / 数据** | 需 **链接器 `ld`** 解析、重定位、拼段后才能运行 |
| **怎么得到** | 汇编源码经 NASM **`-f bin`** 编译，**剥离所有附加信息**，原汁原味输出机器码 | NASM **`-f elf`**（或 `gcc -c`）→ 再 **`ld`** 链接 |

```
helloos.asm（人类可读的汇编文本）
        │
        ▼  nasm -f bin helloos.asm -o ipl.bin
ipl.bin（CPU 可读的裸机器码，无「外壳」）
        │
        ▼  拼进 1.44 MB 软盘 img 偏移 0
helloos.img（带磁盘布局的完整镜像载体）
```

### 2、`-f bin` 与 `-f elf` 对比（帮你不混）

| NASM 输出格式 | 产物特点 | 本课用途 |
|---------------|----------|----------|
| **`-f bin`** | **无结构、无多余元数据** — 平铺的二进制流 | **引导扇区 / IPL** — 恰好契合 BIOS 开机 **直接读 512 字节** 就执行 |
| **`-f elf`** | ELF **目标文件** — 有程序头、段信息 | 需 **`ld` 链接** 成可执行体；**不适合** 当 BIOS 直接加载的引导扇区 |

**引导扇区必须用 `-f bin`：** BIOS 不会帮你做链接 — 它把扇区 **原样** 拷到 **`0x7C00`** 就跳进去执行。

### 3、和 30 天自制 OS 的关联

| 步骤 | 文件 | 说明 |
|------|------|------|
| ① 手写引导 | **`helloos.asm`** | 汇编 **源码**（文本；后缀 **`.asm`**） |
| ② NASM 编译 | **`ipl.bin`** | **512 B 裸二进制**；源码末尾 **`0x55AA`** 魔数 → 文件偏移 **`0x1FE–0x1FF` 为 `55 AA`** |
| ③ 封装进软盘 | **`helloos.img`** | 把 `ipl.bin` **填到 FAT12 规范软盘镜像开头**（1,474,560 B）；后面扇区随 Day 增加 OS 本体、文件系统内容 |
| ④ 模拟启动 | QEMU **`-fda helloos.img`** | 模拟 BIOS 读盘 → 加载引导扇区 → 跑你的 IPL |

### 4、通俗总结（三层）

| 扩展名 | 谁读 | 是什么 |
|--------|------|--------|
| **`.asm`** | **人** | 人类可读的 **汇编源代码**（文本） |
| **`.bin`** | **CPU** | 原始 **机器码**（纯二进制，**无外壳**） |
| **`.img`** | **BIOS / QEMU** | 在 `.bin` 之上，按 **磁盘分区 / 文件系统（FAT12）** 规则封装的 **完整镜像载体** |

Day 2 详述：[section 2.3 · `ipl.bin`](./day-02-asm-makefile/notes/section-2.3-先制作启动区.md) · [section 2.4 · Makefile 拼盘](./day-02-asm-makefile/notes/section-2.4-Makefile-入门.md)

---

## 第一次编译（Day 1）

保存为 **`helloos.asm`**，一行命令出机器码：

```bash
nasm -f bin helloos.asm -o helloos.img
```

| 部分 | 含义 |
|------|------|
| **`helloos.asm`** | 源码；后缀 **`.asm`** |
| **`-f bin`** | 输出 **纯二进制**（引导扇区用，不是 `.obj`/ELF） |
| **`-o helloos.img`** | 写入映像/二进制文件 |

**NASM 替你做的事（不用再像 HxD 手算 hex）：**

- 把 **`mov`、`jmp`、`int`** 等助记符 **编码成机器码**（如 `MOV AX,0` → `B8 00 00`）
- 按 **`ORG`** 处理 **加载地址**；标签、`$` / `$$` 处理 **段内偏移**
- 源码里 **`TIMES 510-($-$$) DB 0`** 自动 **填零到 510 字节**，再 **`DB 0x55, 0xAA`** — 不必手数「还要补多少个 `00` 才到 512 字节」

> **与昨天 HxD 的关系：** 昨天是 **亲手填每一格 hex** 建立直觉；从今天起 **逻辑写进 `.asm`，字节交给 NASM**。用 `nasm -l helloos.lst` 仍可逐字节对照 [HELLOOS_HEX_REFERENCE](./HELLOOS_HEX_REFERENCE.md)。

**完整 1.44 MB 软盘：** `-f bin` 直接产出的大小 = 源码定义的长度（通常先 **512 B 引导扇区**）；嵌入 1.44 MB 模板可交给 Makefile（见 [day-02 section 2.4](./day-02-asm-makefile/notes/section-2.4-Makefile-入门.md)）。

---

## 为什么选 NASM

1. **同一套工具走完全程** — 引导扇区（Day 1–2）→ IPL / bootpack 汇编桩（Day 3+）→ 与 C 链接 → 保护模式切换代码，全用 NASM。
2. **和 GCC、Makefile 自然配合** — `nasm -f bin` 出 `.bin`，`gcc -c` 出 `.o`，`ld` 或书内脚本拼成映像；与 Linux 内核、Bootloader、HFT 底层工程的惯用链一致。
3. **技能可迁移** — 后续读 LKD、写 Linux 模块、改 GRUB/UEFI 相关汇编，文档和示例几乎都是 **NASM/GAS** 生态；不必为一套只在本书出现的工具另学一遍。
4. **输出可对照** — Day 1 手工 HxD 敲的 hex 与 `nasm -l helloos.lst` 列表文件逐字节核对（见 [day-01 section 1.3](./day-01-boot-asm/notes/section-1.3-初次体验汇编程序.md)）。

---

## 和原书 tolset 的关系

| 做法 | 说明 |
|------|------|
| **推荐** | 安装 NASM + GCC + Make + QEMU（见 [SETUP.md](./SETUP.md)） |
| **可选** | 保留 tolset 仅作 **对照**（看作者原始 Makefile、`.nas` 命名） |
| **不必** | 为跟书而必须用 `nask.exe` / `bcc32.exe` |

从原书 `.nas` 移植到 NASM 时，多数指令 **逐行相同**；偶见 nask 专用写法需在 `.lst` 或报错提示下微调（笔记各 Day 会标注）。

---

## 常用命令（示意）

### 汇编引导扇区 / IPL

```bash
# -f bin：纯二进制（引导扇区必加）
# -l：列表文件，对照 HxD 昨天敲的 hex
nasm -f bin helloos.asm -o helloos.img -l helloos.lst
```

### 与 C 协作（Day 3 起）

```bash
nasm -f bin bootpack.asm -o bootpack.bin
gcc -c bootpack.c -o bootpack.o
# 链接布局依当日 Makefile（IPL 读入 bootpack 等）
```

### Makefile 目标链

```makefile
ipl.bin: helloos.asm
	nasm -f bin $< -o $@ -l helloos.lst

helloos.img: ipl.bin
	# 把 ipl.bin 写入 1.44MB 映像偏移 0 …

run: helloos.img
	qemu-system-i386 -fda helloos.img -boot a
```

完整环境步骤见 **[SETUP.md](./SETUP.md)**。

---

## 文件命名约定（本仓库）

| 原书 | 本仓库 |
|------|--------|
| `helloos.nas` | **`helloos.asm`** |
| `nask helloos.nas …` | **`nasm -f bin helloos.asm -o …`** |
| `naskfunc.nas` | **`naskfunc.asm`**（或 `asmfunc.asm`） |
| `helloos.lst` | `nasm -l` 生成 |

**安装 NASM：** [day-01 section 1.3 · 安装 NASM](./day-01-boot-asm/notes/section-1.3-初次体验汇编程序.md#安装-nasm替代-nask)（官网 / Chocolatey / brew）。

---

## 相关

- [SETUP.md](./SETUP.md) — Day 0：QEMU / GCC / Make（NASM 安装见 [day-01 §1.3](./day-01-boot-asm/notes/section-1.3-初次体验汇编程序.md#安装-nasm替代-nask)）
- [LEARNING_PLAN.md](./LEARNING_PLAN.md) — 三阶段学习路径
- [day-01 section 1.3](./day-01-boot-asm/notes/section-1.3-初次体验汇编程序.md) — 汇编 ↔ 机器码
- [day-02 section 2.4](./day-02-asm-makefile/notes/section-2.4-Makefile-入门.md) — Makefile 与 NASM 规则
