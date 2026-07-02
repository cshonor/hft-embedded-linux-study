# 工具链 · NASM + GCC + Make + QEMU

> **学习链路：** VS Code 写 **`.asm`** → **`nasm -f bin`** 出二进制 → **Makefile** 拼软盘映像 → **QEMU** 启动。

---

## 一句话

| 环节 | 工具 |
|------|------|
| 写源码 | **VS Code**（`.asm` 汇编 + 语法高亮） |
| 汇编 | **NASM**（`nasm -f bin …`） |
| 构建 | **GNU Make**（`make ipl` 等） |
| 运行 | **QEMU**（`-fda helloos.img`） |
| C 内核（Day 3+） | **GCC**（MinGW-w64 / MSYS2） |

---

## 汇编：`.asm` + NASM

```
helloos.asm  ──nasm -f bin──►  ipl.bin（512 B）
ipl.bin      ──Makefile────►  helloos.img（1.44 MB 软盘）
helloos.img  ──QEMU────────►  屏幕 hello, world
```

| 命令 | 作用 |
|------|------|
| `nasm -f bin helloos.asm -o ipl.bin` | 汇编 → **512 B** 引导扇区 |
| `nasm -f bin helloos.asm -o ipl.bin -l helloos.lst` | 同上，并生成 **列表文件** 对照 hex |
| `qemu-system-i386 -fda helloos.img -boot a` | 从软盘 A 启动 |

**`-f bin` 必加：** 输出 **无头裸二进制**（不是 `.obj`/ELF）。BIOS 把扇区原样拷到 **`0x7C00`** 就执行，不会帮你做链接。

→ 深入理解 **`.bin` / `-f bin` vs `-f elf` / `.img`**：见 **[`.bin` 是什么？](#bin-是什么-f-bin-vs-f-elf-vs-img)**。

**可练手工程：** [day-02-asm-makefile/code](./day-02-asm-makefile/code/) — `make ipl` 出 `ipl.bin`；拼软盘与 QEMU 见 Day 2 §2.4。

---

## `.bin` 是什么？`-f bin` vs `-f elf` vs `.img`

**结论：`.bin` 泛指原始裸二进制文件。**

### 1、`.bin` 的核心特质

| | `.bin`（裸二进制） | `.o` / ELF 等带格式目标文件 |
|---|-------------------|------------------------------|
| **头部** | **无** | **有** — 程序头、段信息、链接元数据 |
| **每一字节** | **直接就是** CPU 执行的机器指令 / 数据 | 需 **`ld` 链接** 后才能运行 |
| **怎么得到** | `nasm -f bin` | `nasm -f elf` 或 `gcc -c` → `ld` |

### 2、和本课的关联

| 步骤 | 文件 | 说明 |
|------|------|------|
| ① 写源码 | **`helloos.asm`** | 人类可读的汇编文本 |
| ② NASM | **`ipl.bin`** | **512 B**；末尾 **`55 AA`** 启动签名 |
| ③ 拼软盘 | **`helloos.img`** | **1,474,560 B**；`ipl.bin` 写在偏移 0 |
| ④ QEMU | `-fda helloos.img` | 模拟 BIOS 读盘启动 |

### 3、三层扩展名

| 扩展名 | 谁读 | 是什么 |
|--------|------|--------|
| **`.asm`** | **人** | 汇编源代码 |
| **`.bin`** | **CPU** | 裸机器码 |
| **`.img`** | **BIOS / QEMU** | 完整磁盘镜像载体 |

Day 2：[section 2.3 · `ipl.bin`](./day-02-asm-makefile/notes/section-2.3-先制作启动区.md) · [section 2.4 · Makefile](./day-02-asm-makefile/notes/section-2.4-Makefile-入门.md)

---

## 第一次编译

```bash
nasm -f bin helloos.asm -o ipl.bin -l helloos.lst
```

**NASM 替你做的事：**

- `mov` / `jmp` / `int` → 机器码（如 `MOV AX,0` → `B8 00 00`）
- `ORG 0x7C00` 处理加载地址；标签、`$` / `$$` 处理偏移
- `TIMES 510-($-$$) DB 0` 自动填零到 510 字节，再 `DW 0xAA55`

> 昨天 HxD 手填 hex 建立直觉；从今天起 **逻辑写进 `.asm`，字节交给 NASM**。`.lst` 可对照 [HELLOOS_HEX_REFERENCE](./HELLOOS_HEX_REFERENCE.md)。

---

## 为什么选 NASM

1. **同一套工具走完全程** — 引导扇区 → bootpack 汇编桩 → 与 C 链接 → 保护模式切换。
2. **和 GCC、Makefile 自然配合** — 与 Linux 内核、Bootloader、HFT 底层工程惯用链一致。
3. **输出可对照** — `nasm -l helloos.lst` 与 Day 1 手工 hex 逐字节核对。

---

## 与 C 协作（Day 3 起）

```bash
nasm -f bin bootpack.asm -o bootpack.bin
gcc -c bootpack.c -o bootpack.o
# 链接布局依当日 Makefile
```

---

## 文件命名（本仓库）

| 文件 | 说明 |
|------|------|
| `helloos.asm` | Day 1–2 引导扇区源码 |
| `ipl.bin` | NASM 产出的 512 B 启动区 |
| `helloos.img` | 1.44 MB 软盘镜像 |
| `asmfunc.asm` | 汇编辅助函数（如 `io_hlt`），供 C 链接 |
| `helloos.lst` | `nasm -l` 列表文件 |

**安装 NASM：** [day-01 §1.3](./day-01-boot-asm/notes/section-1.3-初次体验汇编程序.md#安装-nasm)

---

## 相关

- [SETUP.md](./SETUP.md) — QEMU / GCC / Make 环境
- [day-02 code/](./day-02-asm-makefile/code/) — `make ipl` 练手
- [LEARNING_PLAN.md](./LEARNING_PLAN.md) — 三阶段路径
