# Ch 2 §3 主存储器与内存映射（索引）

> **MikanOS** · 原书第 2 章 · **🔴**  
> 本章 §3 拆成 **5 篇子笔记**，按顺序读。

---

## 阅读顺序

| # | 笔记 | 带走什么 |
|---|------|----------|
| **3.1** | [内存映射指什么 · RAM 字节视图](./section-3-1-内存映射指什么与RAM视图.md) | 物理 map ≠ 进程 mmap；字节寻址 |
| **3.2** | [RAM 四层占用](./section-3-2-RAM四层占用.md) | 固件 / MMIO / Loader / Conventional |
| **3.3** | [固件 vs EFI 应用 · 内存隔离](./section-3-3-固件与EFI应用内存隔离.md) | **管家 / 工作台 / 办公室** · ExitBootServices |
| **3.4** | [地址清单 · UEFI 内存类型](./section-3-4-地址清单与UEFI内存类型.md) | GetMemoryMap 每条记录 · LoaderCode 属性 |
| **3.5** | [与 mmap 区别 · 自检](./section-3-5-与mmap区别与自检.md) | Ch2 物理摸底 vs Ch19 分页 |

**建议路径：** 3.1 → 3.2 → 3.3（直觉）→ 3.4（对照 CSV）→ 3.5 → [§4 GetMemoryMap](./section-4-GetMemoryMap与导出memmap.md)

---

← [2. EDK II · 索引](./section-2-EDK-II与MikanLoader.md) · 开始 [3.1](./section-3-1-内存映射指什么与RAM视图.md) · 下一章块 [4. GetMemoryMap](./section-4-GetMemoryMap与导出memmap.md)
