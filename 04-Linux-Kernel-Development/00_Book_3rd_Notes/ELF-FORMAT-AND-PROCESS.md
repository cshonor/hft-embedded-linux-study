# ELF 体系梳理（衔接进程模型 · 修正边界误区）

> **用途：** 弄清 ELF 覆盖哪些文件、如何经 `execve` 变成进程，以及「类 Unix ≠ 全是 ELF」。  
> **承接：** [ELF-UEFI-BOOT-CHAIN.md](./ELF-UEFI-BOOT-CHAIN.md)（固件 PE vs 内核后 ELF）· [Ch3 进程](./chapter-03-process-management/) · [Ch5 系统调用](./chapter-05-system-calls/) · [CSAPP 链接/异常](../../01-CSAPP-3rd/)

---

## 一、基础定义

**ELF**（Executable and Linkable Format，可执行与可链接格式）源自 **System V Release 4 Unix**，是开放、跨架构的二进制标准。

| 项 | 值 |
|----|-----|
| 魔数 | `0x7F 45 4C 46`（`\x7fELF`） |
| 位宽 | ELF32 / ELF64 |
| Linux 地位 | 用户态与模块的 **事实标准**（1995 起淘汰主线 `a.out`） |

在 Linux 生态里，ELF **不只是「能双击运行的程序」**，按 `e_type` 至少覆盖：

| `e_type` | 含义 | 典型文件 |
|----------|------|----------|
| **`ET_REL`** | 可重定位 | `.o`（`gcc -c`）；**`.ko` 内核模块**也属此类直觉 |
| **`ET_EXEC`** | 可执行（传统固定装载地址） | 老式静态可执行；调试用 **`vmlinux`** 常为 ELF64 |
| **`ET_DYN`** | 共享目标 / 位置无关 | `.so`；现代 **PIE 可执行文件**也多为 `ET_DYN` |
| **`ET_CORE`** | 核心转储 | `core`（崩溃内存快照） |

---

## 二、流程图：源码 → ELF → 进程

```
  源码 .c/.S
       │  gcc -c
       ▼
  ET_REL  .o ──────────────┐
       │                   │  ld / gcc 链接
       │                   ▼
       │            ET_EXEC 或 ET_DYN（PIE）
       │            （+ 可能依赖 .so = ET_DYN）
       │                   │
       │                   │  写到磁盘：仍是「死文件」
       │                   ▼
       │            execve("/path/to/prog")
       │                   │
       │                   ▼
       │            内核 fs/binfmt_elf.c
       │            按 Program Header mmap 段
       │            （动态则先起动态链接器）
       │                   │
       │                   ▼
       │            重塑当前进程地址空间 / mm
       │            CPU → ELF 入口
       │                   │
       └───────────────────┴──► 运行中的进程（task_struct + 映像）

  旁路：fork() 只复制「已有」进程；不负责从磁盘解析新 ELF。
  日常：fork → 子进程 execve(新 ELF)。
```

| 调用 | 职责 |
|------|------|
| **`fork()`** | 复制已有进程上下文（写时拷贝等） |
| **`execve()`** | **载入 ELF**，毁掉旧用户地址空间合同，换新映像 |

---

## 三、关键链路：磁盘 ELF → 运行中进程

磁盘上的 ELF **本身不会跑**；完整路径：

1. 用户态调用 **`execve()`**  
2. 内核进入 **`fs/binfmt_elf.c`**（ELF 加载器）  
3. 解析文件头与 **程序头表（Program Header）**，把代码/数据等通过映射装进 **当前进程** 虚拟地址空间  
4. 若动态链接：准备栈、指定 **动态链接器**（如 `ld-linux-*.so`）  
5. 更新 **`task_struct` / `mm`**：入口、权限、信号处置等  
6. 旧用户映射释放；CPU 跳到 ELF 入口执行  

### 程序头 vs 节头（必分清）

| 表 | 视角 | 谁用 |
|----|------|------|
| **Program Header（段 Segment）** | **加载** | **内核** `execve` / 加载器 |
| **Section Header（节 Section）** | **链接 / 调试** | 链接器、`readelf -S`、gdb/DWARF |

> 没有可用的程序头（或等价装载信息），内核 **无法按标准路径** `exec` 该文件。

---

## 四、所有类 Unix 程序都是 ELF？❌ 不是

### 现代默认 ELF 的系统

Linux、FreeBSD、OpenBSD、NetBSD、Illumos（Solaris 分支）、Minix 3 等。

| 历史锚点 | |
|----------|--|
| Linux **1.2（1995）** | 主线切到 ELF，淘汰日常 `a.out` |
| 各大 BSD | 约 1998–2000 完成迁移 |

### 不把 ELF 当原生格式的例子

| 系统 | 原生格式 | 纠正 |
|------|----------|------|
| **macOS / iOS（Darwin）** | **Mach-O** | **不是**「ELF 的扩展」。Mach-O 源自 NeXT/Mach 路线，与 ELF **互不原生兼容** |
| 早期 V7 / 4.3BSD | `a.out` | ELF 之前的格式；动态库支持弱 |
| 部分商用 Unix | AIX **XCOFF**、旧 HP-UX **SOM** 等 | 另一套传统 |

**边界一句话：** ELF 是现代 **开源类 Unix（Linux + 主流 BSD）** 的事实标准，**不是**「凡类 Unix 必须 ELF」的强制标准。

---

## 五、与 UEFI 的边界（再钉一次）

| 阶段 | 二进制世界 |
|------|------------|
| 内核已启动之后 | 用户态、`.ko`、`vmlinux` → **ELF** |
| UEFI Boot Services | 固件只认 **PE32+ `.efi`** |
| `grubx64.efi` 等 | 纯 PE |
| `bzImage` + `EFI_STUB` | 外层 PE，内核载荷在 Stub 交接后进入 Linux 世界 |

细节 → [ELF-UEFI-BOOT-CHAIN.md](./ELF-UEFI-BOOT-CHAIN.md)

---

## 六、ELF 相对 `a.out` 的优势（为何胜出）

1. 原生友好 **PIC/PIE**，撑起动态共享库  
2. 架构中立：ELF32/64，覆盖 x86_64 / ARM / RISC-V…  
3. **段供加载、节供链接调试**，职责清晰  
4. 可扩展（DWARF、自定义节等）

---

## 七、上机命令

```bash
file /bin/bash
readelf -h /bin/bash      # 文件头（含 e_type）
readelf -l /bin/bash      # 程序头 — 内核加载看这个
readelf -S /bin/bash      # 节头 — 链接/符号/调试
```

| 期望 | |
|------|--|
| `file` | 出现 `ELF` |
| PIE 常见 | `e_type` 为 **`DYN`（Shared object）** 也正常 — 现代可执行常这样 |

---

## 八、思考题简答（内核路线）

### 1. 没有程序头表，内核还能加载执行吗？

**按标准 ELF `execve` 路径：不能**（或无从得知该映射哪些段、入口在哪）。  
节头再全也主要服务链接/调试；**加载合同是 Program Header**。特殊/定制加载器另说，不是通用用户态模型。

### 2. PIE（多为 `ET_DYN`）和传统 `ET_EXEC` 在 VA 布局上有何区别？

| | 传统 `ET_EXEC` | PIE（`ET_DYN` 可执行） |
|--|----------------|------------------------|
| 装载基址 | 链接时基本定死 | **运行时**选基址（ASLR） |
| 安全性 | 地址更可预测 | 地址空间布局随机化更强 |
| 与 `.so` | 可执行常固定，库可 PIC | 可执行本身也位置无关 |

### 3. `binfmt` 除了 ELF 还支持什么？

内核通过 **`binfmt_*`** 注册多种格式，常见包括：

| 类型 | 说明 |
|------|------|
| **ELF** | `binfmt_elf`（主流） |
| **script** | `#!` 解释器脚本 |
| **misc** | `binfmt_misc`（可注册 Wine、QEMU 用户态等） |
| 遗留/小众 | 历史上的 `a.out`、部分嵌入式 flat 等（视配置） |

模块 `.ko` **不是** `execve`/`binfmt` 路径，而是 **`init_module` / 模块加载器** 解析的 ELF。

---

## 九、一页记忆卡

```
ELF = Linux/BSD 世界的二进制合同
磁盘 ELF --execve--> binfmt_elf --Program Header--> 进程映像
fork 复制进程；exec 换 ELF 映像
macOS = Mach-O（不是 ELF 亲戚）
UEFI = PE；与 ELF 分阶段、分加载器
```

→ [Ch3 §3.1 进程概念](./chapter-03-process-management/notes/section-3.1-进程的概念.md) · [ELF-UEFI-BOOT-CHAIN](./ELF-UEFI-BOOT-CHAIN.md) · [LEARNING-PATH-LOCKED](../../LEARNING-PATH-LOCKED.md)
