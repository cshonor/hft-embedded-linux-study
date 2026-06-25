# Day 2 · Makefile 参考

| 文件 | 说明 |
|------|------|
| [Makefile](./Makefile) | **第一天极简版** — `make` → `os-image.bin`（≈512 B）；`make clean` 清理 |

**用法：**

1. 把 Day 1 的引导扇区汇编放在同目录，文件名 **`helloos.nas`**（**不必改 `.nas` 后缀**）
2. 终端 `cd` 到本目录 → **`make`**（内部等价于 `nasm -f bin helloos.nas -o os-image.bin`）

完整说明：[section-2.4](../notes/section-2.4-Makefile-入门.md) · nask→NASM 对照：[Day 1 §1.3](../day-01-boot-asm/notes/section-1.3-初次体验汇编程序.md#只换-nask--nasm命令对照后缀照旧)
