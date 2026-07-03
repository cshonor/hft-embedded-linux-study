# 工程 01 · 裸 C + Clang（**推荐先做**）

---

## 1. 要解决什么目标？

| 目标 | 说明 |
|------|------|
| **屏幕上出现 Hello World** | UEFI 固件加载你的程序，在控制台打印一行字 |
| **理解最小启动链** | `C 源码` → `目标文件 .o` → `PE 可执行 BOOTX64.EFI` → `FAT 盘 esp/` → `QEMU/真机` |
| **不依赖 EDK II** | 暂不装几 GB 的 EDK II 构建树，也不装 gnu-efi 包 |
| **为 Ch2 打地基** | 先弄清 **EfiMain 入口、SystemTable、ConOut**；Ch2 再用完整 **`<Uefi.h>`** |

---

## 2. 思路是什么？（为什么不用 `<Uefi.h>`？）

正常 UEFI 开发会写：

```c
#include <Uefi.h>   // 几千行：类型、协议、库函数……
EFI_STATUS EFIAPI efi_main(...) { ... }
```

**本工程故意不写这一行。** 这是 MikanOS / os-from-zero **第一章的教法** — **裸 C**：

1. **只声明本章用到的类型** — `CHAR16`、`EFI_SYSTEM_TABLE`、`ConOut->OutputString` 等，见 `hello.c` 顶部
2. **入口仍叫 `EfiMain`** — 和 UEFI 规范一致；链接时用 `/entry:EfiMain` 告诉链接器从哪开始执行
3. **用工具链把 C 编成 PE** — Clang 产出 COFF 的 `.o`，`lld-link` 加上 `subsystem:efi_application` 变成 `.EFI`
4. **自己摆 FAT 目录** — 把 `BOOTX64.EFI` 放到 `esp/EFI/BOOT/`，固件按标准路径去找

**和 `<Uefi.h>` 的关系：**

| | 本工程（裸 C） | 工程 02 / Ch2 |
|---|----------------|---------------|
| 头文件 | 手写 ~30 行结构体 | `#include <Uefi.h>` |
| 入口函数 | `EfiMain` | 02 用 `efi_main`（gnu-efi 包装）；Ch2 用 EDK 模块入口 |
| 依赖 | 仅 Clang + lld-link | gnu-efi 或 EDK II 整套库 |

跑通本工程后，你会知道 **`<Uefi.h>` 本质上就是在帮你声明这些类型和协议** — 再进 [02-gnu-efi-gcc](../02-gnu-efi-gcc/) 或 Ch2 就不陌生了。

→ 笔记：[§2 C + Makefile](../../notes/section-2-二进制编辑器与BOOTX64.md) · [§7 全链路](../../notes/section-7-Ch1裸C与Ch2-EDKII全链路.md)

---

## 3. 一步一步怎么做

### 步骤 0 · 准备环境

- **Windows：** 安装 LLVM（`clang`、`lld-link` 在 PATH 里）→ [SETUP.md](../../../SETUP.md) 路径 A
- **WSL/Linux：** `sudo apt install llvm lld qemu-system-x86 ovmf`

### 步骤 1 · 读源码 `hello.c`

打开 `hello.c`，按顺序看：

1. **类型定义**（第 5–23 行）— 替代 `<Uefi.h>` 的最小声明
2. **`EfiMain` 入口**（第 25 行）— 固件传入 `ImageHandle` 和 `SystemTable`
3. **打印** — `SystemTable->ConOut->OutputString(..., L"Hello, world!\n")`
4. **`while(1)`** — 简单占住 CPU，防止函数返回后固件行为未定义

### 步骤 2 · 编译：`hello.c` → `hello.o`

把 C 编译成**目标文件**（机器码 + 符号表，还不是可执行程序）：

**Windows：**

```powershell
cd …\chapter-01-hello-world\code\01-clang-minimal

clang -target x86_64-pc-win32-coff -ffreestanding -fshort-wchar -c hello.c -o hello.o
```

| 参数 | 作用 |
|------|------|
| `-target x86_64-pc-win32-coff` | 产出 **COFF** 格式 `.o`（Windows 上给 `lld-link` 用） |
| `-ffreestanding` | 不链接 libc，UEFI 环境没有标准 C 库 |
| `-fshort-wchar` | `L"..."` 宽字符 = 2 字节，匹配 UEFI 的 **CHAR16** |
| `-c` | 只编译，不链接 |

**WSL：** 同目录下 `make` 会自动执行等价命令（ELF 目标 + `ld.lld`）。

### 步骤 3 · 链接：`hello.o` → `BOOTX64.EFI`

把 `.o` 链接成 **PE32+ 格式的 UEFI 应用程序**：

**Windows：**

```powershell
New-Item -ItemType Directory -Force -Path esp\EFI\BOOT | Out-Null

lld-link /subsystem:efi_application /entry:EfiMain hello.o /out:esp\EFI\BOOT\BOOTX64.EFI
```

| 参数 | 作用 |
|------|------|
| `/subsystem:efi_application` | 标记为 UEFI 应用（不是 Windows `.exe`） |
| `/entry:EfiMain` | 程序入口 = 你在 `hello.c` 里写的函数 |
| `esp\EFI\BOOT\` | UEFI 约定：x64 默认从该路径加载 **BOOTX64.EFI** |

检查：`dir esp\EFI\BOOT\BOOTX64.EFI` — 文件应存在且大小 > 0。

### 步骤 4 · （可选）QEMU 运行

把 `esp/` 当作 FAT 盘挂给 QEMU，用 OVMF 固件启动：

**Windows：**

```powershell
$esp = (Resolve-Path esp).Path
$ovmf = "C:\Program Files\qemu\share\edk2\x64\OVMF_CODE.fd"   # 按本机路径改

qemu-system-x86_64 -bios $ovmf -drive "format=raw,file=fat:rw:$esp" -m 512M
```

**WSL：**

```bash
make run
```

**预期结果：** 控制台出现 `Hello, world!`

### 步骤 5 · 清理重来

```powershell
Remove-Item hello.o -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force esp -ErrorAction SilentlyContinue
```

或 WSL：`make clean`

---

## 4. 目录里每个文件是什么

| 文件 | 角色 |
|------|------|
| **`hello.c`** | 唯一源码 — 裸 C，无 `<Uefi.h>` |
| **`Makefile`** | WSL/Linux 一键 `make` / `make run` / `make clean` |
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
| 想体验 **真 `<Uefi.h>`** | [02-gnu-efi-gcc](../02-gnu-efi-gcc/)（WSL） |
| 想走 **EDK II 工业流程** | [Ch2 MikanLoader](../../../chapter-02-edk2-memmap/) |
| 想搞懂 PE / 字符串编码 | [§4 结构与编码](../../notes/section-4-计算机结构与编码.md) · [§6 文件格式](../../notes/section-6-C语言过渡与文件格式.md) |

← 返回 [code/ 总索引](../README.md)
