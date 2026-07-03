# Ch2 · EDK II MikanLoader 源码

> **环境：** **WSL2 + Ubuntu**（与 Ch1 相同，见 [SETUP.md](../../SETUP.md)）  
> **本章代码范围：** Hello + `GetMemoryMap` + 导出 **`memmap` CSV** — 不含 Ch3 的 GOP / `kernel.elf`。

---

## 目录结构

```
code/
├── README.md                 ← 本文件（Linux 构建步骤）
└── MikanLoaderPkg/
    ├── Main.c                ← UefiMain：Hello + 内存图 + 写 CSV
    ├── memory_map.hpp        ← MemoryMap 结构（本地副本）
    ├── Loader.inf            ← 单模块描述
    ├── MikanLoaderPkg.dsc    ← 平台 / 库依赖
    └── MikanLoaderPkg.dec    ← Package 声明
```

**与 Ch1 对比：**

| | Ch1 [01-clang-minimal](../../chapter-01-hello-world/code/01-clang-minimal/) | 本目录 |
|---|-------------|--------|
| 头文件 | 手写 `uefi.h` 片段 | **`#include <Uefi.h>`**（MdePkg） |
| 构建 | `clang` + `lld-link` + Makefile | **EDK II** `build` + `.inf/.dsc` |
| 入口 | `EfiMain` | **`UefiMain`**（库包装入口） |
| 本章功能 | 只 Print Hello | Hello + **GetMemoryMap** + **memmap** |

---

## 构建步骤（WSL · 手动 · 不跑 Ansible）

### 0. 依赖（若 Ch1 已装可跳过大部分）

```bash
sudo apt update
sudo apt install -y git build-essential python3 nasm uuid-dev clang lld bc flex bison
```

### 1. 克隆 EDK II（一次性，放 home 目录）

```bash
git clone --recursive https://github.com/tianocore/edk2.git ~/edk2
cd ~/edk2
make -C BaseTools
source edksetup.sh
```

### 2. 把本仓库的 MikanLoaderPkg 链进 edk2

本仓库路径（WSL 下 Windows 盘）示例：

```bash
HFT=/mnt/c/Users/12392/Desktop/hft
PKG=$HFT/08-system-low-level-hands-on/01-mikan-os/chapter-02-edk2-memmap/code/MikanLoaderPkg

cd ~/edk2
ln -sf "$PKG" ./MikanLoaderPkg
ls MikanLoaderPkg/Main.c    # 应能看到
```

### 3. 配置 `Conf/target.txt`

```bash
cd ~/edk2
cat > Conf/target.txt <<'EOF'
ACTIVE_PLATFORM       = MikanLoaderPkg/MikanLoaderPkg.dsc
TARGET                = DEBUG
TARGET_ARCH           = X64
TOOL_CHAIN_TAG        = CLANGPDB
EOF
```

> 若 `CLANGPDB` 报错，可改 `TOOL_CHAIN_TAG = CLANG38`（原书默认）。

### 4. 编译

```bash
cd ~/edk2
source edksetup.sh
build
```

成功产物：

```bash
ls Build/MikanLoaderX64/DEBUG_CLANGPDB/X64/Loader.efi
# 或 DEBUG_CLANG38，取决于 TOOL_CHAIN_TAG
```

### 5. 部署到 FAT / QEMU（与 Ch1 类似）

```bash
LOADER=~/edk2/Build/MikanLoaderX64/DEBUG_CLANGPDB/X64/Loader.efi
ESP=/tmp/mikan-esp
mkdir -p "$ESP/EFI/BOOT"
cp "$LOADER" "$ESP/EFI/BOOT/BOOTX64.EFI"

qemu-system-x86_64 \
  -m 128M -bios /usr/share/ovmf/OVMF.fd \
  -drive if=pide,format=raw,file=fat:rw:$ESP
```

预期：控制台 **`Hello, Mikan World!`**；ESP 根目录生成 **`memmap`**（CSV）。

---

## 常见问题

| 现象 | 处理 |
|------|------|
| `RegisterFilterLib is not found` | 已在 `MikanLoaderPkg.dsc` 加入 `RegisterFilterLibNull` |
| `CLANGPDB` 找不到 | `which clang lld-link`；或改 `CLANG38` |
| QEMU 找不到 OVMF | Ubuntu 24.04：`/usr/share/ovmf/OVMF.fd`（见 Ch1 SETUP） |
| `failed to open root directory` | 确认 `-drive file=fat:rw:...` 挂载了含 EFI 分区的目录 |

---

## 相关笔记

- [§2.2 用 EDK II 重写 Hello World](../notes/section-2-2-用EDK-II重写HelloWorld.md)
- [§4 GetMemoryMap 与导出 memmap](../notes/section-4-GetMemoryMap与导出memmap.md)
- [Ch1 §7 裸 C → EDK II 全链路](../../chapter-01-hello-world/notes/section-7-Ch1裸C与Ch2-EDKII全链路.md)
- [appendix-C `.inf` / `.dsc`](../../appendix-C-edk2-files/)

← [Ch2 README](../README.md)
