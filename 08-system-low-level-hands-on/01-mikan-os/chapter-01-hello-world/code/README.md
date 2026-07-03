# Ch1 · 代码工程索引

> **先看这里，再进子目录。** 全书在 **WSL2 + Ubuntu** 里编译运行（见 [SETUP.md](../../SETUP.md)）。

---

## 三条路线（递进关系）

```
① 裸 C（本仓库默认）     ② GNU-EFI + <Uefi.h>      ③ EDK II（Ch2 起）
   01-clang-minimal          02-gnu-efi-gcc              chapter-02 MikanLoader
   手写最少类型               apt 装 gnu-efi              .inf / .dsc + build
   无 Uefi.h                  真 #include <Uefi.h>        完整 Uefi.h + 官方库
```

| 阶段 | 头文件 | 构建 | 本章工程 |
|------|--------|------|----------|
| **裸 C** | **不** `#include <Uefi.h>` | Clang + **ld.lld** | **[01-clang-minimal/](./01-clang-minimal/)** |
| **GNU-EFI** | **`#include <Uefi.h>`** | GCC + gnu-efi | **[02-gnu-efi-gcc/](./02-gnu-efi-gcc/)** |
| **EDK II** | 完整 **`<Uefi.h>`** | `build` + `.inf/.dsc` | **Ch2** [MikanLoader](../../chapter-02-edk2-memmap/) |

→ 理论：[§7 裸 C → EDK II 全链路](../notes/section-7-Ch1裸C与Ch2-EDKII全链路.md)

---

## 推荐学习顺序

1. 读 **[01-clang-minimal/README.md](./01-clang-minimal/README.md)** — 目标、思路、逐步编译
2. WSL 里 `make run`，QEMU 看到 `Hello, world!`
3. （可选）读 **[02-gnu-efi-gcc/README.md](./02-gnu-efi-gcc/README.md)** — 对比 `#include <Uefi.h>`
4. 读 [§7](../notes/section-7-Ch1裸C与Ch2-EDKII全链路.md)，进 **Ch2 EDK II**

---

## 快速命令（WSL）

```bash
cd chapter-01-hello-world/code/01-clang-minimal
make run
```

```bash
cd chapter-01-hello-world/code/02-gnu-efi-gcc
make run    # 需先 apt install gnu-efi 等
```

---

## 常见困惑

| 你看到的 | 是什么 |
|----------|--------|
| **`hello.c` / `main.c`** | 源码 — 在对应**工程子目录**里 |
| **`hello.o` / `main.o`** | 编译中间产物 — `make` 后才有 |
| **`esp/` / `ESP/`** | FAT 启动盘布局 — 内含 `EFI/BOOT/BOOTX64.EFI` |
| **`BOOTX64.EFI`** | 最终 UEFI 程序 — 固件从 FAT 加载的 PE 文件 |

→ 环境 [SETUP.md](../../SETUP.md) · 理论 [notes/](../notes/)
