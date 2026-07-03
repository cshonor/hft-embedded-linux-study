## 2.4 Boot Services vs Runtime Services

> **§2 子笔记 4/5** · [§2 索引](./section-2-EDK-II与MikanLoader.md)

---

UEFI 固件向应用暴露两大类服务 —— MikanLoader 主要在 **Boot Services 期** 活动。

| 服务 | 阶段 | 本章常用 |
|------|------|----------|
| **Boot Services (`gBS`)** | OS 加载 **之前** | **GetMemoryMap**、内存分配、文件 I/O、协议安装 |
| **Runtime Services (`gRT`)** | OS 运行后 **仍可用** 的部分 | 本章暂不深入（如部分时间/NVRAM 相关） |

**MikanLoader 运行在 Boot Services 期** — 可调用 `gBS` 下各类协议；导出 memmap、读 FAT、分配 Loader 内存都在此阶段完成。

**关键转折点：`ExitBootServices`**

- 调用后 **Boot Services 关闭** — 不能再 `GetMemoryMap`、不能再调大部分 `gBS` API
- Loader 必须在 **Exit 之前** 拿齐内存 map、加载好内核
- 部分 **Runtime** 区内存 **仍保留** — 内核不能全盘当空闲 RAM（见 [3.3 固件 vs EFI 应用](./section-3-3-固件与EFI应用内存隔离.md)）

→ 衔接 Ch1：[EfiMain 与 SystemTable](../../chapter-01-hello-world/notes/section-6-C语言过渡与文件格式.md)  
→ 导出实现：[§4 GetMemoryMap](./section-4-GetMemoryMap与导出memmap.md)

---

← [2.3 MikanLoader 是什么](./section-2-3-MikanLoader是什么.md) · [§2 索引](./section-2-EDK-II与MikanLoader.md) · 下一篇 [2.5 CLANGPDB · 自检](./section-2-5-CLANGPDB与自检.md)
