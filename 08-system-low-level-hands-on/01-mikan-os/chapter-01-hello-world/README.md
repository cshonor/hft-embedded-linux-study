# Ch 1 · 计算机工作原理和 Hello World

> **原书第 1 章** · HFT **🔴** · 官方源码标签 `osbook_day01`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **真正起点：** 在 **没有任何 OS** 的前提下，让屏幕出现 **Hello, world!**

---

### 本章四段结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① C + Makefile** | [code/](./code/)：`make` → `BOOTX64.EFI` | **EfiMain · ConOut** · PE 交叉编译流程 |
| **② 真机 / QEMU** | FAT U 盘或 `make run` | **Secure Boot** 排错 · OVMF 快速迭代 |
| **③ 底层原理** | CPU/内存/I/O · 进制 · 编码 · 小端 | 数字如何变成屏幕上的字符 |
| **④ 格式与 EDK II 预告** | PE/ELF/COFF · Ch2 完整框架 | 工具链分工 · `<Uefi.h>` 演进方向 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. C + Makefile 生成 BOOTX64 | [notes/section-2-二进制编辑器与BOOTX64.md](./notes/section-2-二进制编辑器与BOOTX64.md) · [code/](./code/) |
| 3. 真机与 QEMU 测试 | [notes/section-3-真机与QEMU测试.md](./notes/section-3-真机与QEMU测试.md) |
| 4. 计算机结构与编码 | [notes/section-4-计算机结构与编码.md](./notes/section-4-计算机结构与编码.md) |
| 5. UEFI 启动流程 · BIOS 对比 | [notes/section-5-UEFI启动流程.md](./notes/section-5-UEFI启动流程.md) |
| 6. C 语言过渡与文件格式 | [notes/section-6-C语言过渡与文件格式.md](./notes/section-6-C语言过渡与文件格式.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 本章做了什么？ | **UEFI Hello World** — **C + Makefile** 生成 `BOOTX64.EFI`（Ch2 接 EDK II） |
| 与 02 川合 OS 对照？ | 01：**BIOS · 软盘 · 0x7C00**；Mikan：**UEFI · FAT · BOOTX64.EFI · 长模式** |
| 与 Linux / CSAPP 对照？ | 启动链最前端 — 尚无 syscall/页表；对照 [CSAPP Ch3](../../../01-CSAPP-3rd/chapter-03-machine-level-programs/) 机器级表示 |

**本章目的：** 用 **`make run` 可见成果** 破除 OS 神秘感，建立 **C → PE/COFF → FAT → UEFI 加载** 的现代启动链直觉。

---

## 动手 · code/

```bash
cd chapter-01-hello-world/code
make run    # WSL：需 clang · lld · qemu-system-x86_64 · ovmf
```

→ 详见 [code/README.md](./code/README.md)

---

## 本章学习目标 · 自检

- [ ] 在 [code/](./code/) 用 **`make`** 编译出 **`BOOTX64.EFI`**，**`make run`** 在 QEMU 看到 Hello World
- [ ] 真机失败时知道检查 **Secure Boot**；能用 **QEMU + OVMF** 或 `run_qemu.sh` 验证
- [ ] 说清 **二进制 / 十六进制** 对应关系，以及 **ASCII / UCS-2** 字符编码
- [ ] 描述 **UEFI 从通电到执行 EfiMain** 的启动链
- [ ] 说清 **Clang 编译 → LLD 链接** 与 **PE / ELF / COFF** 格式分工

---

## 相关

- 下一章：[../chapter-02-edk2-memmap/](../chapter-02-edk2-memmap/)
- 对照：[01 Day 1 引导](../../02-30days-os/day-01-boot-asm/) · [附录 F ASCII](../appendix-F-ascii-table/) · [SETUP](../SETUP.md)
- 模块导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
