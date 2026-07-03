# Ch1 · 第一个 BOOTX64.EFI

**纯 LLVM 完全可行** — `llvm` + `lld` 包，**不必装 GCC 交叉链**。

## 推荐：Clang + ld.lld

```bash
sudo apt install -y llvm lld qemu-system-x86 ovmf
cd chapter-01-hello-world/code
make LINK=ld.lld run
```

等价手动命令：

```bash
clang --target=x86_64-elf -ffreestanding -c hello.c -o hello.o
ld.lld -flavor link -subsystem:efi_application -entry:EfiMain hello.o -o bootX64.efi
```

| 步骤 | 命令 |
|------|------|
| 编译 | `clang --target=x86_64-elf -ffreestanding -c hello.c -o hello.o` |
| 链接 | `ld.lld -flavor link -subsystem:efi_application -entry:EfiMain hello.o -o bootX64.efi` |

## 备选：官方 day01 路径（lld-link）

```bash
make run    # clang -target x86_64-pc-win32-coff + lld-link
```

## 环境

| 环境 | 说明 |
|------|------|
| **WSL2（本仓库主路径）** | Windows 上 Ubuntu，`/mnt/c/...` 访问本仓库 |
| **原生 Linux** | 同上 `apt install`，命令一致 |

Ch2 EDK II 全程 LLVM：平台 DSC 里 **`TOOL_CHAIN_TAG = CLANGPDB`** → [Ch2 §2 CLANGPDB](../../chapter-02-edk2-memmap/notes/section-2-EDK-II与MikanLoader.md#五全程-llvmedk-ii-与-clangpdb)

## 文件

| 文件 | 说明 |
|------|------|
| [hello.c](./hello.c) | UEFI C 源码 → 编译后才是 `.efi` |
| [Makefile](./Makefile) | 默认 `lld-link`；**`make LINK=ld.lld`** 纯 LLVM |

→ [§2 工具链](../notes/section-2-二进制编辑器与BOOTX64.md)
