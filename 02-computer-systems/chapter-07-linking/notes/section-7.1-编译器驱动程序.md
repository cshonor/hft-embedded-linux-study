## 7.1 编译器驱动程序

> **Ch7 §7.1** · [章导读](../README.md) · 上节 — · 下节 [§7.2 →](./section-7.2-静态链接.md)

---

`gcc` 等是 **驱动 (driver)**，背后调用 cpp、**cc1**、**as**、**ld**：

- **`as` / `ld` 不属于 GCC** — 它们是 **GNU Binutils** 独立工具；gcc 只是调度它们  
- **`cc1`** 才是 GCC 自带的 C→汇编前端  

细节与四步拆解 → [Ch3 §3.2.1](../../chapter-03-machine-level-programs/notes/section-3.2.1-机器级代码与编译链路.md)

```bash
gcc -Og -Wall -o prog main.c sum.c
# 等价于：预处理 → cc1 → as → ld（你看不见底层进程）
```

常用分解：

```bash
gcc -c main.c -o main.o    # 只到目标文件（内部仍调 as）
gcc main.o sum.o -o prog   # 驱动去调 ld（并补上 crt/libc 等）
```

**HFT：** CI 显式 `-c` 再 link，便于 **缓存 object**、LTO 统一链接阶段；看指令用 Binutils 的 **`objdump -d`**。

---

### 口述巩固 · 自测

1. `as`/`ld` 是 gcc 内置的吗？各属于哪套软件？  
2. `gcc test.c -o test` 背后大致调了哪几步？

---

← — · [本章导读](../README.md) · [§7.2 →](./section-7.2-静态链接.md)
