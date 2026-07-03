# MikanOS · 环境搭建

> 官方附录 A + [os-from-zero README](https://github.com/uchan-nos/os-from-zero) · 细节随笔记更新

**Ch1 Hello World：** Windows 原生 LLVM **即可**，不必 WSL。下面命令 **全部手敲**，无脚本。

---

## 路径 A · Windows 原生 LLVM

### 1. 安装 LLVM

**winget（推荐）**

```powershell
winget install LLVM.LLVM --accept-package-agreements --accept-source-agreements
```

**或官网：** [LLVM Releases](https://github.com/llvm/llvm-project/releases) → `LLVM-xx.x.x-win64.exe` → 勾选 **Add LLVM to the system PATH** → **重开 PowerShell**。

**验证**

```powershell
clang --version
lld-link /?
```

（本机已安装：**LLVM 22.1.8**，`C:\Program Files\LLVM\bin`。若刚装完当前窗口找不到 `clang`，先 **重开终端**，或临时执行：  
`$env:Path = "C:\Program Files\LLVM\bin;" + $env:Path`）

---

### 2. 编译第一个 BOOTX64.EFI

在 PowerShell 中 **逐条执行**：

```powershell
cd C:\Users\12392\Desktop\hft\08-system-low-level-hands-on\01-mikan-os\chapter-01-hello-world\code

clang -target x86_64-pc-win32-coff -ffreestanding -fshort-wchar -c hello.c -o hello.o

New-Item -ItemType Directory -Force -Path esp\EFI\BOOT | Out-Null

lld-link /subsystem:efi_application /entry:EfiMain hello.o /out:esp\EFI\BOOT\BOOTX64.EFI
```

**检查**

```powershell
dir esp\EFI\BOOT\BOOTX64.EFI
```

**清理重来**

```powershell
Remove-Item hello.o -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force esp -ErrorAction SilentlyContinue
```

| 步骤 | 命令 | 说明 |
|------|------|------|
| 编译 | `clang -target x86_64-pc-win32-coff -ffreestanding -fshort-wchar -c …` | 产出 **COFF** `.o`，供 `lld-link` 使用 |
| | `-fshort-wchar` | Windows 上必须加，否则 `L"..."` 与 UEFI **CHAR16** 宽度不一致 |
| 建目录 | `New-Item … esp\EFI\BOOT` | UEFI 标准路径 |
| 链接 | `lld-link /subsystem:efi_application /entry:EfiMain …` | `.o` → **BOOTX64.EFI** |

> **为何不用 `--target=x86_64-elf`？** 在 Windows 上那会生成 **ELF** 格式的 `.o`，**`lld-link` 认不了**（会报 `unknown file type`）。WSL/Linux 下才用 `x86_64-elf` + `ld.lld -flavor link`。

---

### 3. QEMU 运行（可选）

**安装**

```powershell
winget install SoftwareFreedomConservancy.QEMU
```

**OVMF：** 在 QEMU 安装目录下搜 `OVMF_CODE.fd`（路径因版本而异）。

**启动**（已编好 `esp\EFI\BOOT\BOOTX64.EFI`）：

```powershell
cd C:\Users\12392\Desktop\hft\08-system-low-level-hands-on\01-mikan-os\chapter-01-hello-world\code
$esp = (Resolve-Path esp).Path
$ovmf = "C:\Program Files\qemu\share\edk2\x64\OVMF_CODE.fd"   # 改成你的路径

qemu-system-x86_64 -bios $ovmf -drive "format=raw,file=fat:rw:$esp" -m 512M
```

---

## 路径 B · WSL2 / Linux（可选）

```bash
sudo apt install -y llvm lld qemu-system-x86 ovmf
cd /mnt/c/Users/12392/Desktop/hft/08-system-low-level-hands-on/01-mikan-os/chapter-01-hello-world/code

clang --target=x86_64-elf -ffreestanding -fshort-wchar -c hello.c -o hello.o
mkdir -p esp/EFI/BOOT
ld.lld -flavor link -subsystem:efi_application -entry:EfiMain hello.o -o esp/EFI/BOOT/BOOTX64.EFI

qemu-system-x86_64 -bios /usr/share/OVMF/OVMF_CODE.fd -drive format=raw,file=fat:rw:esp -m 512M
```

---

## 全书 MikanOS（稍后）

Ch2+ 跟 [mikanos-build](https://github.com/uchan-nos/mikanos-build) 时 WSL 往往更顺。Ch1 仅编 EFI，**Windows 原生 LLVM 足够**。

EDK II 全程 LLVM：DSC 里 **`TOOL_CHAIN_TAG = CLANGPDB`**。

---

→ [chapter-01-hello-world](./chapter-01-hello-world/) · [code/README](./chapter-01-hello-world/code/README.md)
