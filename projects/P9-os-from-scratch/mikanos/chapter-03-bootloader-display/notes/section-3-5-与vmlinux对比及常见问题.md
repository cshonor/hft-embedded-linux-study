# Ch 3 §3.5 kernel.elf vs vmlinux / Image · 常见问题

> **MikanOS** · 原书第 3 章

## kernel.elf vs 其他内核镜像

| 文件 | 格式 | 用途 | 能否直接 gdb |
|------|------|------|--------------|
| **`kernel.elf` / `vmlinux`** | **ELF 可执行** | 开发调试、QEMU / U-Boot 加载 | ✅ 完整符号 |
| **Image / zImage / bzImage** | 纯二进制或 **压缩** | Linux **量产**、硬盘引导 | ❌ 无 ELF 元数据 |
| **`kernel.bin`** | `objcopy` 去掉 ELF 头 | 实模式 Bootloader **直接搬运** | ❌ |

**Linux 官方** 不叫 `kernel.elf`，根目录统一 **`vmlinux`** — **格式与用途与教学 OS 的 `kernel.elf` 相同**，只是命名习惯。

---

## 使用场景区分

| 场景 | 典型文件 |
|------|----------|
| **教学 OS（MikanOS、xv6）** | **`kernel.elf`** — QEMU `-kernel` 或自家 Loader |
| **RISC-V 裸机** | OpenSBI + U-Boot 加载 **`kernel.elf`** |
| **Linux 内核开发** | **`vmlinux`** 调试；**`bzImage`** 启动 |
| **嵌入式量产** | ELF → **`.bin` / `uImage` / `zImage`** 烧 Flash |

---

## 常见问题

### 1. Bootloader 无法加载 kernel.elf

| 原因 | 检查 |
|------|------|
| **链接脚本地址冲突** | `readelf -l` 的 **VirtAddr** 是否在 [Ch2 Conventional](../../chapter-02-edk2-memmap/notes/section-3-4-地址清单与UEFI内存类型.md) 内 |
| **不是可执行 ELF** | 误链成 **`.o`** 或 **relocatable** — `readelf -h` 看 **Type: EXEC** |
| **架构不匹配** | ELF32 vs ELF64、机器类型 |
| **无 Program Header** | 链接选项错误 |

### 2. gdb 看不到函数名

- 编译未加 **`-g`**
- 执行过 **`strip`**
- 调试的是 **`kernel.bin`** 而非 **`kernel.elf`**

→ 重新编译，用 **未 strip 的 ELF** 挂 gdb。

### 3. QEMU / 真机运行即崩溃

| 检查项 | |
|--------|---|
| **`.lds` 虚拟地址** | 与 Loader 分配是否一致 |
| **栈空间** | `_stack_top` 是否有效 |
| **BSS 清零** | `p_memsz > p_filesz` 部分 |
| **入口** | `e_entry` 是否在 `.text` 内 |

### 4. Mikan 特有：Loader 读不到文件

- U 盘 FAT 路径 / 文件名不是 **`kernel.elf`**
- **EFI_STATUS** 未检查 — 见 [§5](./section-5-KernelMain与错误处理.md)

---

← [3.4 命令](./section-3-4-readelf调试与常用命令.md) · 下一节 [3.6 MikanLoader 流程](./section-3-6-MikanLoader加载流程.md)
