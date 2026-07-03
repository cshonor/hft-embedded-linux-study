# Ch1 · GNU-EFI 裸 C 路径（Linux · 真 Uefi.h）

> **阶段一进阶：** 在 **不装 EDK II** 的前提下，用 **GNU-EFI** 提供的 `<Uefi.h>` + crt0 + 链接脚本，**GCC 一键** 出 `BOOTX64.EFI`。  
> 本仓库 **默认 Ch1** 是更极简的 [hello.c](../hello.c)（手写结构体 + **Clang**，Windows 友好）→ [§7 三路径对照](../notes/section-7-Ch1裸C与Ch2-EDKII全链路.md#二三路径对照)。

## 环境（Linux / WSL）

```bash
sudo apt install gnu-efi gcc-multilib x86_64-linux-gnu-gcc make
```

## 源码 main.c

```c
#include <Uefi.h>

EFI_STATUS EFIAPI efi_main(
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable)
{
    SystemTable->ConOut->OutputString(
        SystemTable->ConOut,
        L"Bare C BOOTX64.EFI Hello World!\r\n");
    return EFI_SUCCESS;
}
```

| 点 | 说明 |
|----|------|
| **`#include <Uefi.h>`** | 来自 **gnu-efi** 包，非 EDK II |
| **`efi_main`** | GNU-EFI 约定入口（与 EDK 的 **`UefiMain`** 命名不同） |
| **`EFIAPI`** | MS x64 调用约定 |
| **`L"..."` + `\r\n`** | UTF-16LE 宽串；UEFI 控制台常用 CRLF |

## Makefile

见 [Makefile](./Makefile)。核心：

```bash
make          # → BOOTX64.EFI
make esp      # → ESP/EFI/BOOT/BOOTX64.EFI
make run      # → QEMU + OVMF（需 ovmf 包）
```

## 手动命令（与 Makefile 等价）

```bash
# 路径因发行版略有差异，以 Makefile 内 EFI_INC / EFI_LIB 为准
x86_64-linux-gnu-gcc -ffreestanding -fno-stack-protector -fpic -fshort-wchar \
  -mno-red-zone -I/usr/include/x86_64-linux-gnu/efi -c main.c -o main.o

x86_64-linux-gnu-gcc -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main \
  -L/usr/lib/x86_64-linux-gnu/gnuefi \
  /usr/lib/x86_64-linux-gnu/gnuefi/crt0-efi-x86_64.o main.o -o BOOTX64.EFI -lefi -lgnuefi
```

## QEMU

```bash
mkdir -p ESP/EFI/BOOT && cp BOOTX64.EFI ESP/EFI/BOOT/
qemu-system-x86_64 -bios /usr/share/OVMF/OVMF_CODE.fd \
  -drive format=raw,file=fat:rw:ESP -m 256M
```

→ 全链路笔记 [§7](../notes/section-7-Ch1裸C与Ch2-EDKII全链路.md)
