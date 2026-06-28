## 5. UEFI Block I/O 与卷镜像

---

### 一、问题：内核尚无块设备驱动

| 现状 | 缺口 |
|------|------|
| Ch6 **PCI** · Ch12 **USB 键盘** | **无** 专用 USB 大容量 / SSD 驱动 |
| 内核要 **读 FAT** | 需要 **卷字节** 在可访问内存 |

**本章策略：** **引导加载器** 代劳 **物理读盘** — 内核只 **解析内存镜像**。

---

### 二、Block I/O Protocol

**UEFI 提供底层块读写：**

```
EFI_BLOCK_IO_PROTOCOL
  .ReadBlocks(MediaId, LBA, BufferSize, Buffer)
```

| 特点 | 说明 |
|------|------|
| **按 LBA 扇区** | 不解析 FAT — 纯 **块搬运** |
| **引导阶段可用** | Bootloader 仍链 **UEFI 服务** |
| **与 Simple FS 不同** | **不** 走文件路径 API — 整卷或前 **N 扇区** .raw 读入 |

→ [Ch2 UEFI 协议概览](../chapter-02-edk2-memmap/notes/section-4-GetMemoryMap与导出memmap.md)

---

### 三、预读卷镜像（如前 16 MiB）

```
Bootloader:
  1. 定位启动 Block I/O 设备（U 盘 / QEMU 虚拟盘）
  2. ReadBlocks(0, size=16MiB, buffer=phys_mem)
  3. 把 buffer 物理地址 + 大小 传给 kernel（BootInfo / 全局）
Kernel:
  4. 视 buffer 为 FAT 卷 — 解析 BPB — 不再调 UEFI
```

| 优点 | 局限 |
|------|------|
| **快速实现 `ls`** | 卷 **大于预读区** 时远端簇不可见 |
| **绕开自研驱动** | 真机需 **Boot Services 退出前** 读完 |
| **QEMU 开发友好** | Ch18+ 读大 **.elf** 可能需 **扩大预读** 或补驱动 |

---

### 四、与 Ch3 引导链衔接

**kernel.elf 加载** 仍可在 bootloader 用 **Simple FS** 读单个文件；**整卷镜像** 为 **内核 FAT 库** 服务 — **两条路径并存**。

→ [Ch3 ELF 加载](../chapter-03-bootloader-display/notes/section-3-第一个内核与ELF加载.md)

---

← [4. 目录项](./section-4-目录条目结构.md) · 下一节 [6. ls](./section-6-ls命令与小结.md)
