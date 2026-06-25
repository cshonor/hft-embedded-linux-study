# Day 2 · Makefile 参考

| 文件 | 说明 |
|------|------|
| [Makefile](./Makefile) | **第一天极简版** — `make` → `os-image.bin`（≈512 B）；`make clean` 清理 |

**用法：**

1. 把 Day 1 的引导扇区汇编存为同目录下的 **`boot.asm`**（或改 Makefile 依赖为 `helloos.nas`）
2. 终端 `cd` 到本目录 → **`make`**

完整说明（Tab 规则、拼 1.44 MB、`run` 目标）：[section-2.4](../notes/section-2.4-Makefile-入门.md)
