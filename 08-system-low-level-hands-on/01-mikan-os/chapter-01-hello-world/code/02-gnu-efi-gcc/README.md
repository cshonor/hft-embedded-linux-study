# 工程 02 · GNU-EFI + GCC（Linux / WSL）

| 项 | 说明 |
|----|------|
| **是什么** | 用 **真 `<Uefi.h>`** + **gnu-efi 库** + **GCC** 链接出 `BOOTX64.EFI` |
| **依赖** | `gnu-efi` 系统包 — **无 EDK II 构建树** |
| **入口** | **`efi_main`**（GNU-EFI 约定，不是 `EfiMain`） |
| **适合** | 理解 **crt0 / PE 链接 / 标准 Uefi.h** — **仅 Linux/WSL** |
| **对应笔记** | [§7 两阶段全链路 · 阶段 A2](../../notes/section-7-Ch1裸C与Ch2-EDKII全链路.md) |

## 目录里有什么

| 文件 | 角色 |
|------|------|
| **`main.c`** | 源码 — `#include <Uefi.h>` |
| **`Makefile`** | `make` / `make run` |
| **`main.o`** | 编译中间产物（make 后生成） |
| **`BOOTX64.EFI`** | 本目录下的 EFI 可执行文件（make 后生成） |
| **`ESP/`** | 复制到标准路径后的 FAT 布局（`make esp` 后生成） |

## 环境

```bash
sudo apt install gnu-efi gcc-multilib x86_64-linux-gnu-gcc make qemu-system-x86 ovmf
```

## 构建与运行

```bash
cd chapter-01-hello-world/code/02-gnu-efi-gcc
make run
make clean
```

## 和工程 01 的区别

| | **01-clang-minimal** | **02-gnu-efi-gcc（本工程）** |
|---|----------------------|------------------------------|
| 头文件 | 手写最少类型 | **`<Uefi.h>`**（apt 包） |
| 编译器 | **Clang** | **x86_64-linux-gnu-gcc** |
| 入口 | `EfiMain` | `efi_main` |
| Windows | ✅ 推荐 | ❌ 不适用 |

← 返回 [code/ 总索引](../README.md)
