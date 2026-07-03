# Ch1 · 第一个 BOOTX64.EFI

**先选一套工具链。** Ch1 推荐 **Clang + LLD**（WSL 一条 `apt install` 搞定）；跟全书 MikanOS 时再装 **x86_64-elf-gcc**。

## 两套工具链（对应上手）

### Clang + LLD — LLVM 组合 · **先试这套**

| 工具 | 作用 |
|------|------|
| **Clang** | C → 汇编 → **目标文件 `.o`** |
| **LLD** | 轻量高速链接器，把 `.o` 拼成镜像 |

Ch1 链 **PE/EFI** 时 Makefile 调用 **`lld-link`**（LLD 的 PE 前端）。Ch2+ 链 **ELF 内核** 时常见 **`ld.lld`** — 同一 LLVM LLD，链接格式不同。

→ 对 HFT：LLVM 线编译/链接迭代快，适合频繁 **Release 构建** 与读汇编热路径（[CSAPP §3.2.1](../../../01-CSAPP-3rd/chapter-03-machine-level-programs/notes/section-3.1-3.2-历史观点与程序编码.md)）。

### x86_64-elf-gcc — GCC 交叉链 · **Ch2+ 全书工程**

给 **非 Linux 宿主的裸 x86_64** 目标生成代码，用于 MikanOS **64 位内核** 与官方 `build.sh`。**Ch1 Hello 不必先装。**

→ [mikanos-build](https://github.com/uchan-nos/mikanos-build) · `source devenv/buildenv.sh`

## 快速运行（WSL · 只装 Clang 线）

```bash
sudo apt install -y clang lld qemu-system-x86 ovmf
cd chapter-01-hello-world/code
make run
```

QEMU 应出现 `Hello, world!`。

## 文件

| 文件 | 说明 |
|------|------|
| [hello.c](./hello.c) | `EfiMain` · `ConOut->OutputString` |
| [Makefile](./Makefile) | `clang` + **`lld-link`** → `esp/EFI/BOOT/BOOTX64.EFI` |

→ 详讲 [§2.二 两套工具链](../notes/section-2-二进制编辑器与BOOTX64.md#二两套工具链怎么理解上手版)

## 产出布局

```
esp/EFI/BOOT/BOOTX64.EFI    ← PE/COFF，OVMF 从 FAT 加载
```

## 下一步

- [§2 流程拆解](../notes/section-2-二进制编辑器与BOOTX64.md)
- Ch2 **EDK II** → [chapter-02-edk2-memmap](../../chapter-02-edk2-memmap/)
