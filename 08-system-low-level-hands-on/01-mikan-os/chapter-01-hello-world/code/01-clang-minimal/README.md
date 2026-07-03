# 工程 01 · 裸 C + Clang（**推荐先做**）

> **环境：** **WSL2 + Ubuntu**（全书统一）。Windows 只作编辑器；编译与运行见 [SETUP.md](../../../SETUP.md)。

---

## 1. 要解决什么目标？

| 目标 | 说明 |
|------|------|
| **屏幕上出现 Hello World** | UEFI 固件加载你的程序，在控制台打印一行字 |
| **理解最小启动链** | `C 源码` → `目标文件 .o` → `PE 可执行 BOOTX64.EFI` → `FAT 盘 esp/` → `QEMU/真机` |
| **不依赖 EDK II** | 暂不装 EDK II 构建树，也不装 gnu-efi 包 |
| **为 Ch2 打地基** | 先弄清 **EfiMain 入口、SystemTable、ConOut**；Ch2 再用完整 **`<Uefi.h>`** |

---

## 2. 思路是什么？（为什么不用 `<Uefi.h>`？）

正常 UEFI 开发会写：

```c
#include <Uefi.h>   // 几千行：类型、协议、库函数……
EFI_STATUS EFIAPI efi_main(...) { ... }
```

**本工程故意不写这一行。** 这是 MikanOS **第一章的教法** — **裸 C**：

1. **只声明本章用到的类型** — `CHAR16`、`EFI_SYSTEM_TABLE`、`ConOut->OutputString` 等，见 `hello.c` 顶部
2. **入口仍叫 `EfiMain`** — 和 UEFI 规范一致；链接时用 `-entry:EfiMain` 告诉链接器从哪开始执行
3. **用工具链把 C 编成 PE** — Clang 产出 `.o`，`ld.lld -flavor link` 加上 `subsystem:efi_application` 变成 `.EFI`
4. **自己摆 FAT 目录** — 把 `BOOTX64.EFI` 放到 `esp/EFI/BOOT/`，固件按标准路径去找

**和 `<Uefi.h>` 的关系：**

| | 本工程（裸 C） | 工程 02 / Ch2 |
|---|----------------|---------------|
| 头文件 | 手写 ~30 行结构体 | `#include <Uefi.h>` |
| 入口 | `EfiMain` | 02 用 `efi_main`；Ch2 用 `UefiMain` |
| 依赖 | 仅 Clang + ld.lld | gnu-efi 或 EDK II 整套库 |

→ 笔记：[§2 C + Makefile](../../notes/section-2-二进制编辑器与BOOTX64.md) · [§7 全链路](../../notes/section-7-Ch1裸C与Ch2-EDKII全链路.md)

---

## 3. 一步一步怎么做（WSL）

### 步骤 0 · 准备环境

```bash
sudo apt update
sudo apt install -y llvm lld clang make qemu-system-x86 ovmf
```

→ 详 [SETUP.md](../../../SETUP.md)

### 步骤 1 · 读源码 `hello.c`

1. **类型定义** — 替代 `<Uefi.h>` 的最小声明
2. **`EfiMain` 入口** — 固件传入 `ImageHandle` 和 `SystemTable`
3. **打印** — `SystemTable->ConOut->OutputString(..., L"Hello, world!\n")`
4. **`while(1)`** — 占住 CPU，防止函数返回后固件行为未定义

### 步骤 2 · 编译：`hello.c` → `hello.o`

```bash
cd chapter-01-hello-world/code/01-clang-minimal

clang --target=x86_64-elf -ffreestanding -fshort-wchar -c hello.c -o hello.o
```

| 参数 | 作用 |
|------|------|
| `--target=x86_64-elf` | WSL 上 Clang 的交叉三元组 |
| `-ffreestanding` | 不链接 libc，UEFI 环境没有标准 C 库 |
| `-fshort-wchar` | `L"..."` = 2 字节，匹配 UEFI **CHAR16** |
| `-c` | 只编译，不链接 |

### 步骤 3 · 链接：`hello.o` → `BOOTX64.EFI`

```bash
mkdir -p esp/EFI/BOOT
ld.lld -flavor link -subsystem:efi_application -entry:EfiMain \
  hello.o -o esp/EFI/BOOT/BOOTX64.EFI
```

| 参数 | 作用 |
|------|------|
| `-subsystem:efi_application` | 标记为 UEFI 应用（不是 Linux ELF 可执行） |
| `-entry:EfiMain` | 程序入口 = `hello.c` 里的函数 |
| `esp/EFI/BOOT/` | UEFI 约定：x64 默认从该路径加载 **BOOTX64.EFI** |

检查：`ls -l esp/EFI/BOOT/BOOTX64.EFI`

### 步骤 4 · 一键编译与运行

```bash
make          # 步骤 2 + 3
make run      # QEMU + OVMF，看到 Hello, world!
make clean    # 删除 hello.o 和 esp/
```

### 步骤 5 · 清理重来

```bash
make clean
```

---

## 4. 目录里每个文件是什么

| 文件 | 角色 |
|------|------|
| **`hello.c`** | 唯一源码 — 裸 C，无 `<Uefi.h>` |
| **`Makefile`** | `make` / `make run` / `make clean` |
| **`hello.o`** | 步骤 2 产物（编译后才有，勿提交） |
| **`esp/`** | 步骤 3 产物 — FAT 布局 + `BOOTX64.EFI`（勿提交） |

```
esp/
└── EFI/BOOT/
    └── BOOTX64.EFI    ← 固件要加载的文件
```

---

## 5. 下一步

| 做完本工程后 | 去哪 |
|--------------|------|
| 想体验 **真 `<Uefi.h>`** | [02-gnu-efi-gcc](../02-gnu-efi-gcc/) |
| 想走 **EDK II 工业流程** | [Ch2 MikanLoader](../../../chapter-02-edk2-memmap/) |
| 想搞懂 PE / 字符串编码 | [§4 结构与编码](../../notes/section-4-计算机结构与编码.md) · [§6 文件格式](../../notes/section-6-C语言过渡与文件格式.md) |

← 返回 [code/ 总索引](../README.md)
