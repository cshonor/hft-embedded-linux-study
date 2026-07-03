# Ch 3 §3 第一个内核与 ELF 加载（索引）

> **MikanOS** · 原书第 3 章 · **🟡**

本章 §3 拆成 **6 篇子笔记**：先搞懂 **`kernel.elf` 是什么**，再对接 **MikanLoader 怎么加载**。

---

## 阅读顺序

| # | 笔记 | 带走什么 |
|---|------|----------|
| **3.1** | [kernel.elf 基础定义与核心作用](./section-3-1-kernel.elf基础定义与核心作用.md) | ELF 静态链接内核 · 与 `vmlinux` 关系 |
| **3.2** | [ELF 三大结构 · 链接视图 vs 执行视图](./section-3-2-ELF三大结构与链接执行双视图.md) | Header / Program Header / Section Header · **Bootloader 看哪张表** |
| **3.3** | [编译、链接脚本与生成流程](./section-3-3-编译链接脚本与生成流程.md) | `gcc` + `ld` + `kernel.lds` |
| **3.4** | [readelf、QEMU、GDB 常用命令](./section-3-4-readelf调试与常用命令.md) | 开发期怎么查段、怎么断点 |
| **3.5** | [与 vmlinux/Image 对比 · 常见问题](./section-3-5-与vmlinux对比及常见问题.md) | 量产镜像 vs 调试 ELF |
| **3.6** | [MikanLoader 加载 kernel.elf 流程](./section-3-6-MikanLoader加载流程.md) | 读盘 · PT_LOAD · 跳 `e_entry` |

**建议路径：** 3.1 → 3.2（看图）→ 3.3 → 3.4（动手 `readelf`）→ 3.6（回到 Mikan 代码）→ [§4 GOP](./section-4-GOP与帧缓冲区.md)

---

← [2. QEMU](./section-2-QEMU监视器与寄存器.md) · [Ch 3 README](../README.md) · 开始 [3.1](./section-3-1-kernel.elf基础定义与核心作用.md)
