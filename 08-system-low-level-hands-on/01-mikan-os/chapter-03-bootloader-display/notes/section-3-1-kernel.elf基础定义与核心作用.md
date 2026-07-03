# Ch 3 §3.1 kernel.elf 基础定义与核心作用

> **MikanOS** · 原书第 3 章

## 一、基础定义

**`kernel.elf`** = **ELF 格式的静态链接内核可执行文件**。

| 常见场景 | 项目 |
|----------|------|
| **教学 OS** | **MikanOS**、xv6、手写小型内核 |
| **裸机 / 嵌入式** | RISC-V / ARM 实验 |
| **Linux 内核** | 根目录 **`vmlinux`** — 命名不同，**本质同为 ELF 内核** |

**ELF（Executable and Linkable Format）** — 跨架构二进制标准，文件里带齐：

- **程序头 (Program Header)** — Bootloader **加载用**
- **节头 (Section Header)** — 链接 / **gdb 调试用**
- **符号表、调试信息 (DWARF)** — `gdb kernel.elf` 断点用

→ 与 UEFI 的 **PE (`.efi`)** 分工：[Ch1 §6 PE/ELF](../../chapter-01-hello-world/notes/section-6-C语言过渡与文件格式.md)

---

## 二、MikanOS 里的两个可执行文件

| | **MikanLoader** | **kernel.elf** |
|---|-----------------|----------------|
| **格式** | **PE** — UEFI 可执行 | **ELF** |
| **谁跑** | 仍在 **Boot Services** 下 | Loader **跳转后** 自持 |
| **入口** | `EfiMain` | **`KernelMain`** / ELF **`e_entry`** |
| **首版行为** | 读盘、GOP、加载内核 | C++ · 初期 **`hlt` 死循环** |

```cpp
extern "C" void KernelMain(/* 帧缓冲参数 — 见 §5 */) {
    while (true) { __asm__("hlt"); }
}
```

| 指令 | 含义 |
|------|------|
| **`hlt`** | CPU 休眠，等中断唤醒 — 证明 **已跳到独立内核**，不是 UEFI 应用 |

---

## 三、核心作用（为什么用 ELF 而不是裸 `.bin`）

### 1. 可被 Bootloader 解析加载

**U-Boot、MikanLoader、QEMU `-kernel`** 读 **Program Header Table**：

- 哪些段是 **代码 (.text)**、**数据 (.data/.bss)**
- 每段 **文件内偏移**、**加载到内存的虚拟地址**、**权限**（R/W/X）
- 加载完 → 跳到 **`e_entry`**

### 2. 完整调试载体

保留符号 + DWARF → 直接：

```bash
gdb kernel.elf
(gdb) break KernelMain
```

无需像纯 `.bin` 那样 **猜地址**。

### 3. 链接器产物 — 内存布局写在文件里

由 `.o` 经 **`ld` + 链接脚本 `kernel.lds`** 合并：

- 内核 **虚拟基址**
- 栈、BSS、各段边界

**`kernel.lds` 和 `kernel.elf` 一样重要** — 见 [§3.3](./section-3-3-编译链接脚本与生成流程.md)。

---

## 四、与 Ch2 物理内存的关系

Loader 按 Program Header **AllocatePages** 时，只能落在 [Ch2 §3.4 ConventionalMemory](../../chapter-02-edk2-memmap/notes/section-3-4-地址清单与UEFI内存类型.md) 等 **允许区域** — **链接脚本地址** 不能与 **UEFI 固件 / MMIO** 冲突。

---

← [§3 索引](./section-3-第一个内核与ELF加载.md) · 下一节 [3.2 ELF 三大结构](./section-3-2-ELF三大结构与链接执行双视图.md)
