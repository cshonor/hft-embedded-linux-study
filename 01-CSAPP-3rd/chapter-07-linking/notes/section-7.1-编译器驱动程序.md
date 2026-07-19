## 7.1 编译器驱动程序

> **Ch7 §7.1** · [章导读](../README.md) · 上节 — · 下节 [§7.2 →](./section-7.2-静态链接.md)

---

`gcc` 等是 **驱动 (driver)**，背后调用 cpp、cc1、as、ld：

```bash
gcc -Og -Wall -o prog main.c sum.c
# 等价于：编译各 .c → .o → ld 链接
```

常用分解：

```bash
gcc -c main.c -o main.o    # 只到目标文件
gcc main.o sum.o -o prog   # 只链接
```

**HFT：** CI 显式 `-c` 再 link，便于 **缓存 object**、LTO 统一链接阶段。

---

### 口述巩固 · 自测

1. （待口述补）本节核心一句话？

---

← — · [本章导读](../README.md) · [§7.2 →](./section-7.2-静态链接.md)
