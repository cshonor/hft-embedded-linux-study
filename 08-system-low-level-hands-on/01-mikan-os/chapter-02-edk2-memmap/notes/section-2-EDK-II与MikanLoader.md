# Ch 2 §2 EDK II 与 MikanLoader（索引）

> **MikanOS** · 原书第 2 章 · **🔴**  
> 本章 §2 拆成 **5 篇子笔记**，按顺序读。

---

## 阅读顺序

| # | 笔记 | 带走什么 |
|---|------|----------|
| **2.1** | [EDK II 是什么 · 行业定位](./section-2-1-EDK-II是什么与行业定位.md) | 规范 vs 参考实现 · 三类用途 · MdePkg |
| **2.2** | [用 EDK II 重写 Hello World](./section-2-2-用EDK-II重写HelloWorld.md) | `<Uefi.h>` · `.inf` 工程化 · 对比 Ch1 裸 C |
| **2.3** | [MikanLoader 是什么](./section-2-3-MikanLoader是什么.md) | 蜜柑加载器 · Hello + memmap · 后续演进 |
| **2.4** | [Boot vs Runtime 服务](./section-2-4-Boot与Runtime服务.md) | `gBS` / `gRT` · GetMemoryMap 在 Boot 期 |
| **2.5** | [CLANGPDB 工具链 · 自检](./section-2-5-CLANGPDB与自检.md) | WSL 上 EDK II 继续用 Clang · 自测 |

**建议路径：** 2.1 → 2.2 → 2.3 → 2.4 → 2.5 → [§3 内存映射 · 3.1](./section-3-1-内存映射指什么与RAM视图.md)

**交叉：** [Ch1 §7 两阶段全链路](../chapter-01-hello-world/notes/section-7-Ch1裸C与Ch2-EDKII全链路.md) · [appendix-C EDK II 文件](../../appendix-C-edk2-files/) · [SETUP.md](../../SETUP.md)

---

← [1. 本章定位](./section-1-本章定位.md) · 开始 [2.1](./section-2-1-EDK-II是什么与行业定位.md) · 下一章块 [3. 内存映射 · 索引](./section-3-主存储器与内存映射.md)
