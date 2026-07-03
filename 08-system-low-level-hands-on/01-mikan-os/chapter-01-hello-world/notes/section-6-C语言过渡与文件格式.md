## 6. C 语言过渡与可执行文件格式

> 手写机器语言 **不可持续** — 本章末尾用 **C/C++** 重写 Hello World，引入现代工具链。

---

### 一、现代构建流程

```
源码 (.c / .cpp)
    ↓  编译器 Clang
对象文件 (.o / .obj) — 机器码 + 重定位信息
    ↓  链接器 LLD
可执行文件 BOOTX64.EFI
```

| 工具 | 角色 |
|------|------|
| **Clang** | 将 C/C++ 编译为 **目标文件** |
| **LLD** | 合并对象文件、解析符号、生成 **EFI 可执行** |
| **EDK II** | Intel 开源 UEFI 开发包 — 提供头文件、库、构建描述（后续章节深入） |

→ [appendix-C EDK II 文件说明](../../appendix-C-edk2-files/) · [appendix-B 获取 MikanOS](../../appendix-B-get-mikanos/)

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

---

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
