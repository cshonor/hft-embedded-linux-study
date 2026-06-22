## 9.11–9.12 内存错误与小结

### 9.11 C 程序常见内存错误（精选）

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

### 9.12 小结（原书）

- VM = **缓存 + 管理 + 保护**；页表 + TLB + 缺页
- 应用层：**mmap** 映射文件/大缓冲；**malloc** 管堆
- 内存 bug 是 C 永恒主题 — 工具与架构双管齐下

→ [Ch 10 系统级 I/O](../chapter-10-系统级IO.md) · [Ch 12 并发与同步](../chapter-12-并发编程.md)

---

← [本章导读](../README.md)
