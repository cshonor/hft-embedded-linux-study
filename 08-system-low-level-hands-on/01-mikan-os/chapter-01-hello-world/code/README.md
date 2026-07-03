# Ch1 · 第一个 BOOTX64.EFI

**不必 WSL。** Windows 上装 [LLVM 预编译包](https://releases.llvm.org/)（勾选 **Add LLVM to system PATH**），`clang --version` 有输出即可。

## Windows 原生（PowerShell）

```powershell
cd chapter-01-hello-world\code
.\build.ps1          # 产出 esp\EFI\BOOT\BOOTX64.EFI
.\build.ps1 -Run     # 需 QEMU + OVMF
```

手动命令（与 WSL 流程一致，链接用 **`lld-link`**）：

```powershell
clang --target=x86_64-elf -ffreestanding -c hello.c -o hello.o
lld-link /subsystem:efi_application /entry:EfiMain hello.o /out:esp\EFI\BOOT\BOOTX64.EFI
```

## WSL / Linux

```bash
sudo apt install -y llvm lld qemu-system-x86 ovmf
make LINK=ld.lld run    # 链接可用 ld.lld -flavor link
```

## 三环境对照

| | **Windows 原生** | **WSL / Linux** |
|---|------------------|-----------------|
| 安装 | LLVM `.exe`，勾选 PATH | `apt install llvm lld` |
| 编译 | `clang --target=x86_64-elf -ffreestanding -c` | 相同 |
| 链 EFI | **`lld-link /subsystem:efi_application …`** | `lld-link` 或 `ld.lld -flavor link` |
| 一键脚本 | **`build.ps1`** | **`make`** / **`make LINK=ld.lld`** |

Ch2 EDK II：**`TOOL_CHAIN_TAG = CLANGPDB`** → [Ch2 §2](../../chapter-02-edk2-memmap/notes/section-2-EDK-II与MikanLoader.md#五全程-llvmedk-ii-与-clangpdb)

## 文件

| 文件 | 说明 |
|------|------|
| [hello.c](./hello.c) | UEFI C 源码 |
| [build.ps1](./build.ps1) | **Windows** 一键编译 |
| [Makefile](./Makefile) | **WSL/Linux** — `make` · `make LINK=ld.lld` |

→ [SETUP.md](../../SETUP.md) · [§2 工具链](../notes/section-2-二进制编辑器与BOOTX64.md)
