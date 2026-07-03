# 工程 01 · Clang 极简 Hello（**推荐先做**）

| 项 | 说明 |
|----|------|
| **是什么** | 用 **C + Clang + lld-link** 编出第一个 **`BOOTX64.EFI`** |
| **依赖** | **无 EDK II**、**无 gnu-efi** — `hello.c` 里手写最少 UEFI 类型 |
| **入口** | `EfiMain` |
| **适合** | **Windows 原生 LLVM**；WSL 也可 `make` |
| **对应笔记** | [§2 C + Makefile](../../notes/section-2-二进制编辑器与BOOTX64.md) · [SETUP](../../../SETUP.md) |

## 目录里有什么

| 文件 | 角色 |
|------|------|
| **`hello.c`** | 唯一源码 — 调 `ConOut->OutputString` 打印 Hello World |
| **`Makefile`** | 可选一键编译（WSL/Linux）；Windows 也可手敲命令 |
| **`hello.o`** | 编译中间产物（**make 后才有**，勿提交） |
| **`esp/`** | FAT 目录布局 + 最终 **`BOOTX64.EFI`**（**make 后才有**） |

## Windows · PowerShell（本工程目录下）

```powershell
cd …\code\01-clang-minimal

clang -target x86_64-pc-win32-coff -ffreestanding -fshort-wchar -c hello.c -o hello.o
New-Item -ItemType Directory -Force -Path esp\EFI\BOOT | Out-Null
lld-link /subsystem:efi_application /entry:EfiMain hello.o /out:esp\EFI\BOOT\BOOTX64.EFI
```

检查：`dir esp\EFI\BOOT\BOOTX64.EFI`

## WSL / Linux

```bash
cd chapter-01-hello-world/code/01-clang-minimal
make              # 或 make LINK=ld.lld
make run          # 需 ovmf + qemu-system-x86_64
make clean        # 删除 hello.o 和 esp/
```

## 产出物含义

```
esp/                          ← QEMU 当作 FAT 盘挂载的目录
└── EFI/BOOT/
    └── BOOTX64.EFI           ← UEFI 固件要加载的文件（PE32+）
```

## 和其他工程的关系

| 工程 | 区别 |
|------|------|
| **[02-gnu-efi-gcc](../02-gnu-efi-gcc/)** | 用系统包 **`<Uefi.h>`** + **GCC**，仅 Linux/WSL |
| **Ch2 EDK II** | **MikanLoader** — `.inf/.dsc` + `build` |

← 返回 [code/ 总索引](../README.md)
