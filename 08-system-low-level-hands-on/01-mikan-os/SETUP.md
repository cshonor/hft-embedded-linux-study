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
sudo apt install -y clang lld make qemu-system-x86 ovmf git
```

> **Ubuntu 24.04 提示：** 若 `make` 报 `lld-link: not found`，再装 **`lld-18`**（或 `lld` 元包）：  
> `sudo apt install -y lld-18` — Makefile 会自动用 `lld-link-18`。

**验证**

```bash
clang --version
command -v lld-link || command -v lld-link-18
qemu-system-x86_64 --version
ls /usr/share/ovmf/OVMF.fd /usr/share/qemu/OVMF.fd 2>/dev/null | head -1
```

---

## 2. Ch1 · 编译第一个 BOOTX64.EFI

**工程目录：** [01-clang-minimal/](./chapter-01-hello-world/code/01-clang-minimal/) — 完整说明见该目录 **README.md**。

在 WSL 中 **逐条执行**（或 `make` 一键）：

```bash
cd chapter-01-hello-world/code/01-clang-minimal

clang -target x86_64-pc-win32-coff -ffreestanding -fshort-wchar -c hello.c -o hello.o
mkdir -p esp/EFI/BOOT
lld-link /subsystem:efi_application /entry:EfiMain hello.o /out:esp/EFI/BOOT/BOOTX64.EFI
```

（Ubuntu 上可能是 `lld-link-18` 而不是 `lld-link`。）

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
| 编译 | `clang -target x86_64-pc-win32-coff -ffreestanding -fshort-wchar -c …` | 产出 **COFF** `.o`（与官方 day01/c 一致） |
| | `-fshort-wchar` | 必须加，否则 `L"..."` 与 UEFI **CHAR16** 宽度不一致 |
| 链接 | `lld-link /subsystem:efi_application /entry:EfiMain …` | `.o` → **PE32+ BOOTX64.EFI** |
| 运行 | `make run` | 挂载 `esp/` 为 FAT，OVMF 固件启动 |

> **说明：** UEFI x64 应用底层是 **PE32+**。WSL 里仍用 **COFF 对象 + lld-link** 交叉链（与 Windows 上同一套 PE 工具链），只是 **在 Linux 壳里跑**，不必 Windows 原生 LLVM。

---

## 3. Ch1 · QEMU 运行（可选）

已编好 `esp/EFI/BOOT/BOOTX64.EFI` 后：

```bash
cd chapter-01-hello-world/code/01-clang-minimal
make run
```

等价于：

```bash
qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd \
  -drive format=raw,file=fat:rw:esp -m 512M
```

**Ubuntu 24.04 排错**

| 现象 | 原因 | 处理 |
|------|------|------|
| `could not load PC BIOS '...OVMF_CODE_4M.fd'` | 24.04 的 **CODE_4M** 是 **pflash 分体镜像**，不能直接用 `-bios` | 用 **`/usr/share/ovmf/OVMF.fd`**（Makefile 已自动选） |
| `apt install` 报 **404 Not Found** | 包索引过期 | 先 `sudo apt update` 再装 |
| `Could not get lock` | 另一个 apt 进程占锁 | `sudo killall apt apt-get` 后重试 |

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
