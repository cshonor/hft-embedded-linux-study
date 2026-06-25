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

**nask 可以这么理解：** 川合秀实为了方便读者，在 NASM 语法风格上做了定制汇编器。**我们直接用原版 NASM** — 认 **`.nasm`、`.asm`**，**原书 `.nas` 也不用改后缀**。

---

## 后缀名：不用改

原书写法：

```
helloos.nas  ──nask──►  helloos.img
```

**下不了 nask 没关系** — 源码 **继续叫 `helloos.nas` 完全没问题**（存成 `helloos.nasm` 也行）。**只换编译命令**，语法 NASM 能读：

| 原书（nask） | **本仓库（NASM）** |
|--------------|-------------------|
| `nask helloos.nas helloos.img` | `nasm -f bin helloos.nas -o helloos.img` |
| （nask 默认出纯二进制） | **加 `-f bin`**，让 NASM 输出与 nask **相同的纯二进制** |

列表文件（对照 HxD hex）可选：

```bash
nasm -f bin helloos.nas -o helloos.img -l helloos.lst
```

编译结果应与书里、与 Day 1 HxD 手工版 **逐字节一致** — **不必纠结后缀名**。

---

## 第一次编译（Day 1）

保存为 **`helloos.nas`**（或 `helloos.nasm`），一行命令出机器码：

```bash
nasm -f bin helloos.nas -o helloos.img
```

| 部分 | 含义 |
|------|------|
| **`helloos.nas`** | 源码；后缀 `.nas` / `.nasm` / `.asm` 均可 |
| **`-f bin`** | 输出 **纯二进制**（引导扇区用，不是 `.obj`/ELF） |
| **`-o helloos.img`** | 写入映像/二进制文件 |

**NASM 替你做的事（不用再像 HxD 手算 hex）：**

- 把 **`mov`、`jmp`、`int`** 等助记符 **编码成机器码**（如 `MOV AX,0` → `B8 00 00`）
- 按 **`ORG`** 处理 **加载地址**；标签、`$` / `$$` 处理 **段内偏移**
- 源码里 **`TIMES 510-($-$$) DB 0`** 自动 **填零到 510 字节**，再 **`DB 0x55, 0xAA`** — 不必手数「还要补多少个 `00` 才到 512 字节」

> **与昨天 HxD 的关系：** 昨天是 **亲手填每一格 hex** 建立直觉；从今天起 **逻辑写进 `.nasm`，字节交给 NASM**。用 `nasm -l helloos.lst` 仍可逐字节对照 [HELLOOS_HEX_REFERENCE](./HELLOOS_HEX_REFERENCE.md)。

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
nasm -f bin helloos.nasm -o helloos.img -l helloos.lst
```

### 与 C 协作（Day 3 起）

```bash
nasm -f bin bootpack.asm -o bootpack.bin
gcc -c bootpack.c -o bootpack.o
# 链接布局依当日 Makefile（IPL 读入 bootpack 等）
```

### Makefile 目标链

```makefile
ipl.bin: helloos.nasm
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
| `helloos.nas` | **保持 `.nas` 即可**（或 `.nasm` / `.asm`） |
| `nask helloos.nas …` | **`nasm -f bin helloos.nas -o …`** |
| `naskfunc.nas` | 同名 `.nas` 或 `asmfunc.nasm` |
| `helloos.lst` | `nasm -l` 生成 |

**安装 NASM：** [day-01 section 1.3 · 安装 NASM](./day-01-boot-asm/notes/section-1.3-初次体验汇编程序.md#安装-nasm替代-nask)（官网 / Chocolatey / brew）。

---

## 相关

- [SETUP.md](./SETUP.md) — Day 0：QEMU / GCC / Make（NASM 安装见 [day-01 §1.3](./day-01-boot-asm/notes/section-1.3-初次体验汇编程序.md#安装-nasm替代-nask)）
- [LEARNING_PLAN.md](./LEARNING_PLAN.md) — 三阶段学习路径
- [day-01 section 1.3](./day-01-boot-asm/notes/section-1.3-初次体验汇编程序.md) — 汇编 ↔ 机器码
- [day-02 section 2.4](./day-02-asm-makefile/notes/section-2.4-Makefile-入门.md) — Makefile 与 NASM 规则
