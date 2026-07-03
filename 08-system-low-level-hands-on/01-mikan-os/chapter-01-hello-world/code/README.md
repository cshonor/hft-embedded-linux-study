# Ch1 · 第一个 BOOTX64.EFI

全部命令手敲，见 [SETUP.md](../../SETUP.md)。

## Windows · PowerShell（`code\` 目录）

```powershell
clang -target x86_64-pc-win32-coff -ffreestanding -fshort-wchar -c hello.c -o hello.o
New-Item -ItemType Directory -Force -Path esp\EFI\BOOT | Out-Null
lld-link /subsystem:efi_application /entry:EfiMain hello.o /out:esp\EFI\BOOT\BOOTX64.EFI
```

`-fshort-wchar`：Windows 上让 `L"..."` 匹配 UEFI 的 16 位宽字符。  
`-target x86_64-pc-win32-coff`：产出 COFF 对象，**`lld-link` 才能链**（不要用 `x86_64-elf`，会报 `unknown file type`）。

## WSL / Linux

```bash
clang --target=x86_64-elf -ffreestanding -fshort-wchar -c hello.c -o hello.o
mkdir -p esp/EFI/BOOT
ld.lld -flavor link -subsystem:efi_application -entry:EfiMain hello.o -o esp/EFI/BOOT/BOOTX64.EFI
```

## 文件

| 文件 | 说明 |
|------|------|
| [hello.c](./hello.c) | UEFI C 源码 |
| [Makefile](./Makefile) | WSL/Linux 可选 |

→ [SETUP.md](../../SETUP.md)
