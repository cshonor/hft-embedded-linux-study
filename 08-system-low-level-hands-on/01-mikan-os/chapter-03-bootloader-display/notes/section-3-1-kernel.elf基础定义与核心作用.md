# Ch 3 §3.1 kernel.elf 基础定义 · ELF 是什么 · 常见误区

> **MikanOS** · 原书第 3 章

## 先纠正两个关键误区

### 误区 ① 「ELF 是给加载器识别内核用的」

| ❌ 错误说法 | ✅ 修正 |
|------------|--------|
| ELF 专供 Bootloader **认内核** | **ELF 是通用二进制格式** — 描述 **任意** 一段代码/数据在磁盘与内存中的布局 |
| 只有 `kernel.elf` 才是 ELF | **`ls`、你的 Rust 程序、`.so`、`vmlinux`、`.ko`** 在 Linux 上 **全是 ELF** |

**加载器**（U-Boot、**MikanLoader**、`execve` 里的内核加载器）**不管加载的是内核还是普通程序**，都靠 **同一套 ELF 头 + Program Header** 知道：**每段从文件哪读、拷到内存哪、权限是什么**。

`kernel.elf` 只是 **「用 ELF 打包的内核」** —— ELF 的 **一种用途**，不是 ELF 存在的理由。

---

### 误区 ② 「把 ELF 加载出来就是操作系统」

| ❌ 错误说法 | ✅ 修正 |
|------------|--------|
| 读到 `kernel.elf` = OS 已在跑 | **ELF 只是磁盘上的容器** — 内核代码 **打包** 在里面 |
| 加载 = 完成启动 | 必须 **按段拷入内存 + 清 BSS +（通常）ExitBootServices + 跳 `e_entry`**，**CPU 才开始执行内核** |

**类比：** 把游戏 ISO 拷进硬盘 ≠ 游戏在运行；还要 **安装/加载进内存并启动 exe**。

---

## 一、ELF 不只是给内核用

**ELF（Executable and Linkable Format）** = **文件结构标准**，描述二进制 **怎么分段、放哪、怎么加载**。

| ELF 封装的对象 | 例子 |
|----------------|------|
| **用户态程序** | `/bin/ls`、你的量化程序 |
| **动态库** | `.so` |
| **Linux 内核镜像** | **`vmlinux`**（项目里常叫 **`kernel.elf`**） |
| **内核模块** | `.ko` |

**不是专门为内核设计的** — 普通应用 **在 Linux 上默认就是 ELF**。

MikanOS 选 ELF 做 `kernel.elf`，是因为 **工具链成熟**（`ld`、`readelf`、`gdb`），不是因为 ELF **只能** 装内核。

---

## 二、ELF 文件里存的是什么？

**ELF Header（文件头）** 记录全局信息：

| 字段 | 作用 |
|------|------|
| **魔数** | 确认这是 ELF |
| **架构** | x86-64 等 |
| **程序入口 `e_entry`** | CPU **跳转执行的起点** |
| **Program / Section 表偏移** | 分段表、节表在文件中的位置 |

**Program Header Table（程序头 / 分段表）** — **Loader 加载时读这张**：

| 每一段 (Segment) | 典型内容 |
|------------------|----------|
| **`.text`** | 代码 |
| **`.rodata`** | 只读数据 |
| **`.data`** | 已初始化全局数据 |
| **`.bss`** | 未初始化缓冲区（文件中可能不占字节，内存里要清零） |

每条记录包含：**磁盘偏移、加载到内存的地址、段大小、读/写/执行权限**。

→ Loader 读表就知道：**每一块该拷到物理内存哪** — 见 [§3.6 完整流程](./section-3-6-MikanLoader加载流程.md)

---

## 三、加载器 · ELF · 内核 — 三者关系（MikanOS）

```
磁盘上的 kernel.elf（ELF 容器，还不会自己跑）
        │
        ▼
bootx64.efi / MikanLoader（你写的 UEFI 加载器，自己是 .efi / PE）
        │
        ├── ① UEFI 文件 API 读 ELF 进内存缓冲
        ├── ② 解析 ELF Header + Program Header
        ├── ③ 各 PT_LOAD 段 → 拷到 EfiConventionalMemory 对应地址
        ├── ④ 清零 .bss（p_memsz > p_filesz 部分）
        ├── ⑤ ExitBootServices（释放固件临时服务 — 全书后续章节）
        └── ⑥ 跳 e_entry → 内核开始执行
        │
        ▼
操作系统内核（KernelMain…）— 这时才算「内核在跑」
```

| 角色 | 是什么 |
|------|--------|
| **`kernel.elf`** | 磁盘上的 **ELF 文件** — 内核本体 **打包** |
| **`bootx64.efi`** | **操作系统加载器** — UEFI 应用，**固件能直接执行** |
| **你的代码** | **固件不会自动启动 ELF 内核** — 必须 **手写 ELF 解析与搬运** |

---

## 四、和 `.efi` 对比（别混）

| | **`.efi`（PE）** | **`kernel.elf`（ELF）** |
|---|------------------|-------------------------|
| **谁原生能加载** | **UEFI 固件** — 从 FAT 读入即执行 | **UEFI 固件不会** 像 `.efi` 一样自动跑 ELF |
| **运行阶段** | **Linux 启动之前**（固件环境） | 需 **Loader 先载入内存** 再跳转 |
| **Linux 启动之后** | 不再使用 | 用户程序、**vmlinux**、`.ko` **仍是 ELF** — 由 **内核的 execve / 模块加载器** 解析 |
| **Mikan 分工** | **MikanLoader** 自己 | **Loader 读入并解析的对象** |

**EFI 加载器的核心工作：**

> UEFI **只能直接跑 `.efi`**，**不能** 把 `kernel.elf` 当 `.efi` 丢给固件。  
> 所以 **你必须在 Loader 里写**：读文件 → 解析 ELF → 搬段 → 跳入口。

→ [Ch1 §6 PE/ELF](../../chapter-01-hello-world/notes/section-6-C语言过渡与文件格式.md)

---

## 五、`kernel.elf` 在 MikanOS 里具体指什么

**`kernel.elf`** = 用 ELF 格式链接出来的 **静态内核镜像**（教学里常命名 `kernel.elf`；Linux 叫 **`vmlinux`**）。

| | **MikanLoader (.efi)** | **kernel.elf** |
|---|------------------------|----------------|
| **格式** | **PE** | **ELF** |
| **谁加载** | UEFI 固件 | **MikanLoader** |
| **入口** | `EfiMain` | **`e_entry` → `KernelMain`** |
| **首版** | 读盘、GOP、加载 | C++ · 初期 **`hlt`** 证明已跳转 |

```cpp
extern "C" void KernelMain(/* 帧缓冲 — §5 */) {
    while (true) { __asm__("hlt"); }
}
```

---

## 六、为什么内核用 ELF 而不是裸 `.bin`

| 好处 | 说明 |
|------|------|
| **Program Header** | Loader 知 **拷哪、权限** — 不必手写固定偏移表 |
| **调试** | `gdb kernel.elf`、符号、DWARF |
| **链接脚本** | `kernel.ld` 定义布局 → 写进 ELF |

→ 编译链接：[§3.3](./section-3-3-编译链接脚本与生成流程.md) · 双视图：[§3.2](./section-3-2-ELF三大结构与链接执行双视图.md)

---

## 七、与 Ch2 物理内存

Loader **AllocatePages** 时必须落在 [Ch2 ConventionalMemory](../../chapter-02-edk2-memmap/notes/section-3-4-地址清单与UEFI内存类型.md) — **链接脚本地址** 不能踩固件 / MMIO。

---

← [§3 索引](./section-3-第一个内核与ELF加载.md) · 下一节 [3.2 ELF 双视图](./section-3-2-ELF三大结构与链接执行双视图.md)
