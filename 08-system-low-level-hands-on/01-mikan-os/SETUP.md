# MikanOS · 环境搭建

> 官方附录 A + [os-from-zero README](https://github.com/uchan-nos/os-from-zero) · 细节随笔记更新

**全书统一开发环境：WSL2 + Ubuntu。**  
Windows 只作编辑器宿主（Cursor）；**编译、运行、QEMU 均在 WSL 里完成**。下面命令 **全部手敲**，不用官方 Ansible 黑盒脚本。

---

## 0. WSL2 准备（Windows 上一次性）

若尚未安装 WSL：

```powershell
# 在 Windows PowerShell（管理员）执行一次
wsl --install -d Ubuntu
```

装好后打开 **Ubuntu** 终端，后续 **所有 MikanOS 命令都在 WSL 里跑**。

**建议：** 把仓库放在 WSL 家目录（如 `~/hft`），编译比 `/mnt/c/...` 更快、权限更少坑。若代码在 Windows 盘，路径形如：

```bash
cd /mnt/c/Users/12392/Desktop/hft/08-system-low-level-hands-on/01-mikan-os
```

---

## 1. Ch1 · 安装工具（WSL / Ubuntu）

```bash
sudo apt update
sudo apt install -y llvm lld clang make qemu-system-x86 ovmf git
```

**验证**

```bash
clang --version
ld.lld --version
qemu-system-x86_64 --version
ls /usr/share/OVMF/OVMF_CODE.fd
```

---

## 2. Ch1 · 编译第一个 BOOTX64.EFI

**工程目录：** [01-clang-minimal/](./chapter-01-hello-world/code/01-clang-minimal/) — 完整说明见该目录 **README.md**。

在 WSL 中 **逐条执行**（或 `make` 一键）：

```bash
cd chapter-01-hello-world/code/01-clang-minimal

clang --target=x86_64-elf -ffreestanding -fshort-wchar -c hello.c -o hello.o
mkdir -p esp/EFI/BOOT
ld.lld -flavor link -subsystem:efi_application -entry:EfiMain hello.o -o esp/EFI/BOOT/BOOTX64.EFI
```

**或一键：**

```bash
make          # → esp/EFI/BOOT/BOOTX64.EFI
make run      # → QEMU + OVMF
make clean    # 删除 hello.o 和 esp/
```

**检查**

```bash
ls -l esp/EFI/BOOT/BOOTX64.EFI
```

| 步骤 | 命令 | 说明 |
|------|------|------|
| 编译 | `clang --target=x86_64-elf -ffreestanding -fshort-wchar -c …` | 产出 **ELF 三元组** 的 `.o`（WSL 上给 `ld.lld` 用） |
| | `-fshort-wchar` | 必须加，否则 `L"..."` 与 UEFI **CHAR16** 宽度不一致 |
| 链接 | `ld.lld -flavor link -subsystem:efi_application -entry:EfiMain …` | `.o` → **PE32+ BOOTX64.EFI** |
| 运行 | `make run` | 挂载 `esp/` 为 FAT，OVMF 固件启动 |

> **说明：** UEFI x64 应用底层是 **PE32+**；在 WSL 里用 **`clang --target=x86_64-elf` 编译 + `ld.lld -flavor link` 链接**，即可产出固件可加载的 `.EFI`，不必在 Windows 原生装 LLVM。

---

## 3. Ch1 · QEMU 运行（可选）

已编好 `esp/EFI/BOOT/BOOTX64.EFI` 后：

```bash
cd chapter-01-hello-world/code/01-clang-minimal
make run
```

等价于：

```bash
qemu-system-x86_64 -bios /usr/share/OVMF/OVMF_CODE.fd \
  -drive format=raw,file=fat:rw:esp -m 512M
```

---

## 4. 全书 MikanOS（Ch2 起 · 手动搭链）

**不跑官方 `mikanos-build` 的 Ansible 一键脚本** — 按章节自己装、自己理解每一步。

| 阶段 | 需要什么 | 怎么装（WSL） |
|------|----------|---------------|
| **Ch2 Loader** | **EDK II** + BaseTools | `git clone https://github.com/tianocore/edk2.git ~/edk2` → `make -C BaseTools` → `source edksetup.sh` |
| **Ch2 工具链** | **Clang + LLD**（与 Ch1 一致） | 已在 §1 装好；DSC 里 **`TOOL_CHAIN_TAG = CLANGPDB`** |
| **Ch3+ 内核** | **x86_64-elf-gcc** 交叉链 + Newlib 等 | `sudo apt install gcc-x86-64-elf` 或按章节自行编译 Newlib（笔记随 Ch 更新） |
| **运行** | **QEMU + OVMF** | 已在 §1 装好 |

**源码仓库（按需 clone，不必 ansible）：**

```bash
git clone https://github.com/uchan-nos/mikanos.git ~/mikanos
# Ch2：在 ~/edk2 下 ln -s ~/mikanos/MikanLoaderPkg ./
```

→ Ch2 详述：[chapter-02-edk2-memmap/](./chapter-02-edk2-memmap/) · [§7 全链路](./chapter-01-hello-world/notes/section-7-Ch1裸C与Ch2-EDKII全链路.md)

---

→ [chapter-01-hello-world](./chapter-01-hello-world/) · [code/README](./chapter-01-hello-world/code/README.md)
