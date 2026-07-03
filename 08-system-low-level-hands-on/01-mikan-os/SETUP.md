# MikanOS · 环境搭建

> 官方附录 A + [os-from-zero README](https://github.com/uchan-nos/os-from-zero) · 细节随笔记更新

**Ch1 Hello World 两条等价路径（二选一）：**

| 路径 | 环境 | 适合 |
|------|------|------|
| **A · Windows 原生 LLVM** | [LLVM 官网](https://releases.llvm.org/) 预编译包 + PowerShell | **不必装 WSL** |
| **B · WSL2 / Linux** | `apt install llvm lld` + `make` | 与官方脚本、EDK II 更顺 |

---

## 路径 A · Windows 原生 LLVM（推荐先试）

### 1. 安装 LLVM

1. 打开 [LLVM Releases](https://github.com/llvm/llvm-project/releases) 或 [releases.llvm.org](https://releases.llvm.org/) 下载 **Windows 预编译安装包**（如 `LLVM-xx.x.x-win64.exe`）
2. 安装时 **勾选 “Add LLVM to the system PATH for all users/current user”**
3. **重开** PowerShell / cmd，验证：

```powershell
clang --version
lld-link /?
```

能输出版本即成功。

### 2. 编译 Ch1 EFI（PowerShell）

```powershell
cd C:\Users\12392\Desktop\hft\08-system-low-level-hands-on\01-mikan-os\chapter-01-hello-world\code
.\build.ps1
```

或手动（与 WSL 流程 **基本一致**，链接用 Windows 自带的 **`lld-link`**）：

```powershell
clang --target=x86_64-elf -ffreestanding -c hello.c -o hello.o
mkdir -Force esp\EFI\BOOT | Out-Null
lld-link /subsystem:efi_application /entry:EfiMain hello.o /out:esp\EFI\BOOT\BOOTX64.EFI
```

| 步骤 | Windows 原生 | WSL / Linux |
|------|--------------|-------------|
| 编译 | `clang --target=x86_64-elf -ffreestanding -c` | 相同 |
| 链接 PE/EFI | **`lld-link /subsystem:efi_application …`** | `lld-link` 或 `ld.lld -flavor link …` |

### 3. QEMU（可选 · 本机跑 Hello World）

安装 [QEMU for Windows](https://www.qemu.org/download/#windows) 并准备 **OVMF** 固件，然后：

```powershell
.\build.ps1 -Run
```

OVMF 路径因安装包而异；脚本会尝试常见位置，找不到则需手动 `-bios`（见 [code/build.ps1](./chapter-01-hello-world/code/build.ps1)）。

---

## 路径 B · WSL2 / Linux

| 组件 | 用途 |
|------|------|
| **WSL2** (Ubuntu 22.04+) | Linux 用户态 + apt |
| **llvm + lld** | 纯 LLVM；`make LINK=ld.lld` |
| **QEMU** + **OVMF** | UEFI 模拟 |

```bash
sudo apt update
sudo apt install -y llvm lld qemu-system-x86 ovmf
cd …/chapter-01-hello-world/code
make LINK=ld.lld run
```

→ [chapter-01-hello-world/code/](./chapter-01-hello-world/code/README.md)

---

## 全书 MikanOS（可选 · 稍后）

跟官方 [mikanos-build](https://github.com/uchan-nos/mikanos-build) 时，**WSL 通常更省事**（`buildenv.sh`、挂载 FAT 镜像）。Ch1 仅编 EFI 时 **Windows 原生 LLVM 足够**。

Ch2 EDK II 全程 LLVM：DSC 里 **`TOOL_CHAIN_TAG = CLANGPDB`**。

---

## QEMU + OVMF（WSL 示例）

```bash
qemu-system-x86_64 \
  -bios /usr/share/OVMF/OVMF_CODE.fd \
  -drive format=raw,file=fat:rw:esp \
  -m 512M
```

> **与 02 30days-os 差异：** 那边 `qemu-system-i386 -fda`（BIOS 软盘）；MikanOS 用 **x86_64 + UEFI**。

## 路径建议

工程放 **纯英文路径**（与 [02-30days-os SETUP](../02-30days-os/SETUP.md) 相同），例如 `C:\dev\hft\` 或 `D:\dev\mikanos\`。

---

环境跑通后，在 [chapter-01-hello-world](./chapter-01-hello-world/) 起记笔记。
