# Ch5 Debugging Kernel Memory Issues - Part 1

> Part 2: Instrumentation & Memory Debugging · 🔴 精读

内核内存错误检测：KASAN (地址消毒器)、UBSAN (未定义行为消毒器)、SLUB debug (slab 泄漏检测)、kmemleak (内核内存泄漏检测)。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 5.1 内核内存错误的类型 | `notes/section-5-1.md` |
| 5.2 KASAN：地址消毒器 (越界 / use-after-free) | `notes/section-5-2.md` |
| 5.3 UBSAN：未定义行为检测 | `notes/section-5-3.md` |
| 5.4 SLUB debug：slab 分配器调试 | `notes/section-5-4.md` |
| 5.5 kmemleak：内核内存泄漏检测 | `notes/section-5-5.md` |

---

## HFT 关联

精读。写自定义内核模块时，KASAN 能在开发期捕获 90% 的内存错误。树莓派 5 ARM64 完整支持 KASAN，需 CONFIG_KASAN=y 重编译内核。
