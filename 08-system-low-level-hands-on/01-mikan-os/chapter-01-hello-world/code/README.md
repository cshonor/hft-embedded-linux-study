# Ch1 · 代码工程索引

> **先看这里再进子目录。** 第一章有 **两个独立小工程**，各编各的 `BOOTX64.EFI`，**不要混文件**。

## 选哪个？

| 工程 | 目录 | 何时用 |
|------|------|--------|
| **01 · Clang 极简** | **[01-clang-minimal/](./01-clang-minimal/)** | **默认。** Windows 已装 LLVM，或 WSL 想最快跑通 |
| **02 · GNU-EFI + GCC** | **[02-gnu-efi-gcc/](./02-gnu-efi-gcc/)** | Linux/WSL，想用 **真 `<Uefi.h>`** + gcc 理解链接细节 |

**Ch2 及以后：** EDK II **MikanLoader** — 不在本目录，见 [chapter-02](../../chapter-02-edk2-memmap/) · [§7 全链路](../notes/section-7-Ch1裸C与Ch2-EDKII全链路.md)。

---

## 各工程 README

| 工程 | 说明文档 |
|------|----------|
| 01 Clang 极简 | [01-clang-minimal/README.md](./01-clang-minimal/README.md) |
| 02 GNU-EFI | [02-gnu-efi-gcc/README.md](./02-gnu-efi-gcc/README.md) |

---

## 常见困惑

| 你看到的 | 是什么 |
|----------|--------|
| **`hello.c` / `main.c`** | **源码** — 在对应工程子目录里，不在 `code/` 根目录 |
| **`hello.o` / `main.o`** | **编译产物** — `make` 或 `clang -c` 后才有，已在各工程 `.gitignore` 里 |
| **`esp/` / `ESP/`** | **FAT 启动盘布局** — 内含 `EFI/BOOT/BOOTX64.EFI`，QEMU 挂载用 |
| **`BOOTX64.EFI`** | **最终 UEFI 程序** — 固件从 FAT 加载的文件 |

**不要在 `code/` 根目录直接找 Makefile** — 请进入 **`01-clang-minimal/`** 或 **`02-gnu-efi-gcc/`**。

---

## 快速命令

**Windows（工程 01）：**

```powershell
cd 01-clang-minimal
# 见 01-clang-minimal/README.md
```

**WSL（工程 01）：**

```bash
cd 01-clang-minimal && make run
```

**WSL（工程 02）：**

```bash
cd 02-gnu-efi-gcc && make run
```

→ 环境安装 [SETUP.md](../../SETUP.md) · 理论 [notes/](../notes/)
