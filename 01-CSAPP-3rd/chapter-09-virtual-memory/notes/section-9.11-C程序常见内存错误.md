## 9.11 C 程序常见内存错误（精选）

> **Ch9 §9.11** · [章导读](../README.md) · 上节 [§9.10 ←](./section-9.10-垃圾收集.md) · 下节 [§9.12 →](./section-9.12-小结.md)

---

| 错误 | 后果 | 防范 |
|------|------|------|
| **坏指针解引用** | segfault | 初始化、检查 NULL |
| **读未初始化** | 随机行为 | `calloc`、值初始化 |
| **栈缓冲区溢出** | 安全漏洞 | 边界检查、`fgets`（→ [Ch 3](../chapter-03-machine-level-programs/notes/section-3.10-指针调试与缓冲区溢出.md)） |
| **off-by-one** | 踩边界 | 循环 `< n`、分配 `n+1` |
| **指针/对象混淆** | 逻辑错 | `sizeof(*p)` vs `sizeof(p)` |
| **指针算术错** | 越界 | `p+i` 类型缩放 |
| **悬空引用** | UAF | 释放后置 NULL；Rust 编译期防 |
| **重复 free / 漏 free** | 崩溃/泄漏 | 所有权清晰；ASan/Valgrind |

```bash
# 开发期
gcc -fsanitize=address -g ...
valgrind --leak-check=full ./prog
```

**HFT：** CI 跑 **ASan/UBSan** 在测试二进制；生产靠 **代码规范 + 池化**；Rust 策略层减 UAF 类 bug。

---

### 口述巩固 · 自测

1. （待口述补）本节核心一句话？

---

← [§9.10 ←](./section-9.10-垃圾收集.md) · [本章导读](../README.md) · [§9.12 →](./section-9.12-小结.md)
