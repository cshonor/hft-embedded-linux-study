## 5.5 Ch2 内存衔接 · 自检

> **§5 子笔记 5/5** · [§5 索引](./section-5-UEFI启动流程.md)

---

### 固件 vs EFI 应用 · 内存布局（详读 Ch2）

Ch1 启动七步里 **⑤ 加载镜像、⑥ EfiMain** 发生在 RAM 里 —— 但 **「固件区」和「你的 `.efi` 区」如何分开**，属于 **物理内存摸底**，在 **Ch2 §3** 展开：

| 比喻 | 对应 |
|------|------|
| **管家办公区** | UEFI 固件（Runtime / Boot Services / ACPI…） |
| **客厅工作台** | 已加载的 `BOOTX64.EFI`（LoaderCode / LoaderData） |
| **自己的办公室** | 内核加载进 Conventional RAM |
| **拆工作台** | **ExitBootServices** 后回收 Loader 占用空间 |

→ **[Ch2 §3.3 固件 vs EFI 应用 · 内存隔离](../../chapter-02-edk2-memmap/notes/section-3-3-固件与EFI应用内存隔离.md)**  
→ Type 枚举与生命周期：[Ch2 §3.4](../../chapter-02-edk2-memmap/notes/section-3-4-地址清单与UEFI内存类型.md)

---

### §5 自检（口述巩固）

1. **UEFI 去哪找引导文件？** — **FAT ESP** 上固定路径 **`/EFI/BOOT/BOOTX64.EFI`**，不是 512B 扇区。  
2. **BIOS 传统启动认文件系统吗？** — **不认**，只执行 **0 扇区 IPL**（校验 **0xAA55**）。  
3. **BOOTX64.EFI 等于 IPL 吗？** — **功能等价**（第一级 OS 加载器），**形态不同** — 完整 `.efi` PE 程序，非 512B 扇区。  
4. **UEFI 和 HTTP 类比成立吗？** — **成立** — 都只定规则；Nginx/OVMF 才是实现。  
5. **EDK II 产出什么？** — **整板固件镜像**（BIOS/OVMF）+ **独立 `.efi`**（驱动/引导器）。  
6. **MikanOS 为何用 UEFI？** — **64 位长模式 + C/LLVM 开发**，跳过实模式扇区汇编。  
7. **QEMU 如何模拟 UEFI？** — **OVMF 固件** + FAT 虚拟盘挂载含 `BOOTX64.EFI` 的目录。  
8. **固件和 BOOTX64.EFI 在内存里什么关系？** — 见 [Ch2 §3.3](../../chapter-02-edk2-memmap/notes/section-3-3-固件与EFI应用内存隔离.md)（**管家 / 工作台 / 办公室**）。

---

← [5.4 关键名词](./section-5-4-关键名词与本章位置.md) · [§5 索引](./section-5-UEFI启动流程.md) · 下一节 [6. C 与文件格式](./section-6-C语言过渡与文件格式.md)
