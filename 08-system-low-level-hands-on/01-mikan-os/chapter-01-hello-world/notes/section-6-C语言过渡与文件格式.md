## 6. C 语言过渡与可执行文件格式

> **核心认识：** **`BOOTX64.EFI` 不是另一种语言** — 就是 **符合 UEFI 规范的 C 程序**，经 **交叉编译器 + 链接器** 生成的可执行文件。你写 `EfiMain`、调固件服务；**PE 格式细节** 交给 Makefile / 工具链。

---

### 一、现代构建流程

```
hello.c（C 源码 · UEFI 业务逻辑）
    ↓  交叉编译器（Clang 或 x86_64-elf-gcc）
COFF 对象 (.o) — 机器码 + 重定位信息
    ↓  链接器（lld-link 或 EDK II / 官方脚本）
BOOTX64.EFI — PE32+ 形态，固件可加载
```

| 工具 | 角色 |
|------|------|
| **Clang** | Ch1 默认；`-target x86_64-pc-win32-coff` → **PE 对象**；UEFI 开发 **更省心** |
| **x86_64-elf-gcc** | MikanOS [mikanos-build](https://github.com/uchan-nos/mikanos-build) 推荐交叉链；Ch2+ 内核 / Loader 与 **`buildenv.sh`** 同环境 |
| **LLD / EDK II `build`** | 链接成 **`.efi`**；**`/subsystem:efi_application`** 等由 Makefile 写好，无需自写链接脚本 |
| **EDK II** | Ch2 起：`<Uefi.h>`、库、工程描述 — 在交叉链之上再包一层规范 |

→ 工具链对照 [§2.二](./section-2-二进制编辑器与BOOTX64.md#二用哪些交叉编译器) · [code/Makefile](../code/Makefile)

---

### 二、UEFI 程序入口：`EfiMain()`

C 语言版本中，入口由 UEFI 规范定义：

```c
EFI_STATUS EfiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    // 通过 SystemTable->ConOut 等输出 "Hello, world!"
    return EFI_SUCCESS;
}
```

| 参数 | 含义 |
|------|------|
| **ImageHandle** | 当前镜像句柄 |
| **SystemTable** | 固件提供的 **系统服务表** — 控制台、内存、启动服务等 |

**对比：** 用户态 C 程序常见 `main(int argc, char **argv)` — UEFI 阶段 **尚无 argc/argv**，由 **SystemTable** 访问固件能力。

**和 Linux 程序的对照：**

| | **Linux 用户态** | **UEFI 应用（本章）** |
|---|------------------|------------------------|
| 入口 | `main()` | **`EfiMain()`** |
| 源码语言 | C / C++ | **同样是 C / C++** |
| 链接产物 | **ELF** | **PE（`.efi`）** |
| 谁加载 | 内核 + 动态链接器 | **UEFI 固件** 从 FAT 读入 |

→ [appendix-C EDK II 文件说明](../../appendix-C-edk2-files/) · [appendix-B 获取 MikanOS](../../appendix-B-get-mikanos/)

### 三、专栏：常见机器语言文件格式

| 格式 | 平台 / 用途 | 说明 |
|------|-------------|------|
| **PE** | **Windows** · **UEFI x64 应用底层** | Portable Executable — `.efi` 在 x64 PC 上实质为 **PE32+** |
| **ELF** | **Linux** 等 | Executable and Linkable Format — 常见 `.so` / 可执行文件 |
| **COFF** | **中间对象** | Common Object File Format — **.o** 对象文件常用；链接前格式 |

**关系链（简化）：**

```
C 源码 → 编译 → COFF 对象 (.o)
              → 链接 → PE（BOOTX64.EFI）或 ELF（Linux 二进制）
```

| 本章 | 格式 |
|------|------|
| 手写 / 链接产物 | **PE 形态的 BOOTX64.EFI** |
| 编译中间产物 | **COFF 对象** |

→ 与 [CSAPP Ch3 ELF 节区](../../../01-CSAPP-3rd/chapter-03-machine-level-programs/) 对照（结构细节不同，**「头 + 节区 + 符号」** 思想相通）

---

### 四、本章收束

```
C + Makefile → BOOTX64.EFI（[code/](../code/)）  →  先跑通再拆解 ConOut
        ↓
CPU / 编码 / UEFI 启动链                        →  理解「谁在什么时候运行」
        ↓
Ch 2：EDK II + MikanLoader + 内存 map            →  `<Uefi.h>` 工程化
```

---

### 五、索引

| Ch1 主题 | 继续读 |
|----------|--------|
| EDK II · 内存 map | [chapter-02-edk2-memmap](../chapter-02-edk2-memmap/) 🔴 |
| BIOS 软盘对照 | [01 Day 1](../../02-30days-os/day-01-boot-asm/) |
| ASCII | [appendix-F-ascii-table](../../appendix-F-ascii-table/) |
| 开发环境 | [SETUP.md](../../SETUP.md) · [appendix-A-dev-env](../../appendix-A-dev-env/) |

---

← [5. UEFI 启动](./section-5-UEFI启动流程.md) · [Ch 1 导读](../README.md) · [Ch 2 EDK II](../../chapter-02-edk2-memmap/)
