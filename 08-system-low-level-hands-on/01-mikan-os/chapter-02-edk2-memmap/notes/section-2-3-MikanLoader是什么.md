## 2.3 MikanLoader 是什么

> **§2 子笔记 3/5** · [§2 索引](./section-2-EDK-II与MikanLoader.md)

---

| 名称 | 含义 |
|------|------|
| **MikanLoader** | 「蜜柑加载器」— 本书 OS 的 **Boot Loader 雏形**（EDK II 工程里的 UEFI 应用模块） |
| **当前能力（Ch2）** | 仍输出 Hello World + **导出 UEFI 内存映射 CSV** |
| **后续演进（Ch3+）** | 加载 **kernel.elf**、初始化显示与硬件 — 仍属 Loader 职责 |

```
MikanLoader.efi（UEFI 阶段 · Boot Services 期）
    ↓ 未来
MikanOS 内核（Ch 8+ 内存管理、Ch 19 分页…）
```

**在全书中的位置：**

| 章 | Loader 做什么 |
|----|---------------|
| **Ch1** | 裸 C `BOOTX64.EFI` — 只打印，理解工具链 |
| **Ch2** | **MikanLoader** — EDK II 工程化 + **GetMemoryMap** |
| **Ch3+** | Loader 加载内核、传参、跳转入内核 |

→ 内存摸底：[§3 内存映射 · 3.1](./section-3-1-内存映射指什么与RAM视图.md)  
→ 下一章：[Ch3 引导加载器与显示](../../chapter-03-bootloader-display/)

---

← [2.2 用 EDK II 重写 Hello World](./section-2-2-用EDK-II重写HelloWorld.md) · [§2 索引](./section-2-EDK-II与MikanLoader.md) · 下一篇 [2.4 Boot vs Runtime 服务](./section-2-4-Boot与Runtime服务.md)
