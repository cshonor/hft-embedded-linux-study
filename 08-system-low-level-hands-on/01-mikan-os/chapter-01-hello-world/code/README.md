# Ch1 · 第一个 BOOTX64.EFI

**不必从零手写整个 EFI 文件。** 本章动手路径：写符合 UEFI 规范的 **C 代码** → **Makefile 一键交叉编译** → 产出 **PE 格式** `BOOTX64.EFI` → 放进 **FAT** 目录树 → UEFI 固件识别并加载。

## 文件

| 文件 | 说明 |
|------|------|
| [hello.c](./hello.c) | 最小模板：`EfiMain` · 经 `ConOut->OutputString` 打印 |
| [Makefile](./Makefile) | `clang` + `lld-link` → `esp/EFI/BOOT/BOOTX64.EFI` |

与官方 [mikanos-build/day01/c](https://github.com/uchan-nos/mikanos-build/tree/master/day01/c) 对齐；完整 devenv（`make_image.sh` · `run_qemu.sh`）见 [appendix-B](../../appendix-B-get-mikanos/) · [SETUP.md](../../SETUP.md)。

## 快速运行（WSL）

```bash
sudo apt install -y clang lld qemu-system-x86 ovmf
cd chapter-01-hello-world/code
make run
```

QEMU 窗口应出现 `Hello, world!`。

## 产出布局

```
esp/
└── EFI/
    └── BOOT/
        └── BOOTX64.EFI    ← lld-link 生成的 PE/COFF
```

`make run` 用 QEMU **`-drive file=fat:rw:esp`** 把该目录当作 FAT 卷挂载，OVMF 固件按规范加载其中的 `BOOTX64.EFI`。

## 下一步

- 笔记 [§2 流程拆解](../notes/section-2-二进制编辑器与BOOTX64.md)
- Ch2 起引入完整 **EDK II** · `<Uefi.h>` → [chapter-02-edk2-memmap](../../chapter-02-edk2-memmap/)
