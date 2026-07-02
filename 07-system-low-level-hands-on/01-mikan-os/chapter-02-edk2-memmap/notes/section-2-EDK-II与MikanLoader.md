## 2. EDK II 与 MikanLoader

---

### 一、EDK II 是什么

**EDK II（EFI Development Kit II）** — Intel 开发并 **开源** 的 UEFI 软件开发工具包。

| 用途 | 说明 |
|------|------|
| **UEFI 固件（BIOS）开发** | 平台厂商定制固件 |
| **UEFI 应用程序** | 如本章 **MikanLoader**、安装器、诊断工具 |
| **提供** | 头文件、库、构建系统、协议定义 |

→ 环境搭建 [SETUP.md](../../SETUP.md) · 文件结构 [appendix-C EDK II](../../appendix-C-edk2-files/)

---

### 二、用 EDK II 重写 Hello World

Ch 1 末尾已用 C + Clang/LLD 写出 Hello World；本章改用 EDK II **基础库**：

```c
#include <Uefi.h>
// … 使用 EFI_SYSTEM_TABLE、ConOut 等已有抽象
```

| 对比 Ch 1 裸 C | EDK II 版 |
|----------------|-----------|
| 手动处理更多细节 | **`<Uefi.h>`** 统一类型与协议 |
| 代码冗长 | **更简洁、可维护** |
| 实验程序 | 纳入 **MikanOS 工程结构** |

**关键头文件：** `<Uefi.h>` — 引入 UEFI 规范中的基础类型、协议、服务表声明。

---

### 三、MikanLoader 命名

| 名称 | 含义 |
|------|------|
| **MikanLoader** | 「蜜柑加载器」— 本书 OS 的 **Boot Loader 雏形** |
| **当前能力** | Hello World + **内存映射导出**（本章） |
| **后续演进** | Ch 3 起加载内核、初始化硬件 — 仍属 Loader 职责 |

```
MikanLoader.efi（UEFI 阶段）
    ↓ 未来
MikanOS 内核（Ch 8+ 内存管理、Ch 19 分页…）
```

---

### 四、Boot Services vs Runtime Services（概念）

| 服务 | 阶段 | 本章常用 |
|------|------|----------|
| **Boot Services (`gBS`)** | OS 加载 **之前** | **GetMemoryMap**、内存分配、文件 I/O |
| **Runtime Services (`gRT`)** | OS 运行后仍可用部分 | 本章暂不深入 |

MikanLoader 运行在 **Boot Services 期** — 可调用 `gBS` 下各类协议。

→ 衔接 [Ch 1 EfiMain](../chapter-01-hello-world/notes/section-6-C语言过渡与文件格式.md)

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. 内存映射](./section-3-主存储器与内存映射.md)
