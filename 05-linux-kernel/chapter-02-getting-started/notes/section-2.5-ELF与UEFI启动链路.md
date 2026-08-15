## ⑤ ELF 与 UEFI 启动链路 · Boot Chain（拓展）

> **拓展节（非原书独立章）：** 分清固件 **PE32+** 与内核接管后 **ELF**；承接 §2.3 镜像产物与 Ch3 `exec`。
> 范围：x86_64 UEFI + Linux 为主；ARM64 逻辑同构。

---
## 一、核心结论

1. **UEFI 固件原生加载器只认 PE32/PE32+（`.efi`），不原生支持 ELF。**
2. **Linux 运行起来之后**（内核态 + 用户态）的原生二进制标准是 **ELF64/ELF32**。
3. 两套格式 = **两套加载规范、两套解析器**；分界线是：**Linux 内核是否已完成硬件接管**（Boot Services 结束之后）。

```
UEFI / Boot Services 世界          Linux 已接管世界
─────────────────────            ─────────────────
PE32+ .efi                       ELF（vmlinux 概念、用户程序、.so、.ko）
固件镜像加载器                      内核 binfmt / 模块加载器
无 task_struct / 无 fork           有进程模型、fork/exec
```

---

## 二、两种格式对照

| 项目 | ELF | PE32+/PE32（UEFI `.efi`） |
|------|-----|---------------------------|
| 标准归属 | System V Unix 衍生；Linux / BSD 等 | COFF 扩展；微软系；**UEFI 规范强制** |
| 魔数 | `\x7fELF`（`0x7F 'E' 'L' 'F'`） | `MZ`（`0x4D 0x5A`，DOS 头） |
| 主要环境 | 内核已运行后的加载 | 固件 **Boot Service** 阶段 |
| 解析主体 | 用户态：`fs/binfmt_elf.c`；模块：模块加载器 | UEFI 固件内置镜像加载器 |
| 文件类型 | 可执行、`.o`、`.so`、coredump、`.ko` | UEFI 应用 / 驱动 / 引导器 |
| 重定位直觉 | Program Header、PLT/GOT 等 | PE Section、导入导出表 |
| 典型文件 | `/bin/bash`、`vmlinux`、`libc.so.6`、`*.ko` | `bootx64.efi`、`grubx64.efi`、`systemd-bootx64.efi` |

---

## 三、极易混淆的三层镜像

### ① `vmlinux` — 原始内核镜像

| 属性 | 说明 |
|------|------|
| 格式 | 典型为 **ELF64**（含段、符号，便于调试） |
| UEFI | **不能** 直接当 `.efi` 扔进 ESP 让固件加载 |

### ② `bzImage` / `vmlinuz`（开启 `CONFIG_EFI_STUB`）

| 层 | 角色 |
|----|------|
| 外层 | 对 UEFI 表现为 **合法 PE32+**（可被固件加载） |
| 内层载荷 | 真正的内核映像（教学上可理解为「ELF 内核被包进 PE 壳」） |
| 流程 | 固件按 PE 载入 → 跑 **EFI Stub** → 初始化/交接 → 跳进内核入口 |

> **通俗：** `EFI_STUB` = 给内核套一层 UEFI 看得懂的 PE「外包装」。  
> 细节随架构/配置变化；读源码时搜 `CONFIG_EFI_STUB`、`arch/x86/boot` 相关即可，不必死记布局字节。

### ③ GRUB / systemd-boot（`*.efi`）

| 属性 | 说明 |
|------|------|
| 格式 | **纯 PE32+**，不是「外面 PE、里面嵌套一整颗 ELF 引导器」那种教学模型 |
| 角色 | 固件先加载引导器；**之后由引导器**读磁盘上的内核（可解析非 PE 的内核镜像） |

---

## 四、两条常见启动路径（勿混成一条）

### 路径 A · 固件 → 引导器 → 内核（最常见桌面/服务器）

```
上电 → UEFI 固件
  → 扫 ESP，加载 grubx64.efi / systemd-bootx64.efi   【PE32+】
  → 引导器仍处 UEFI 环境（Boot Services 可用）
  → 引导器读取磁盘上的 vmlinuz/bzImage（+ initramfs）
  → 跳转内核入口
  → 内核初始化 mm / 驱动 / 调度 …
  → ExitBootServices：销毁 Boot Services，脱离固件加载世界
  → 挂根文件系统，执行 /sbin/init（systemd 等）                 【ELF64】
  → init 不断 fork + execve 拉用户态
  → 用户进程 / .so / .ko 均为 ELF 体系
```

### 路径 B · 固件直接加载 EFI Stub 内核（可无 GRUB）

```
上电 → UEFI
  → 直接加载带 EFI_STUB 的内核镜像（外层 PE）
  → Stub 跑完 → 进内核入口
  → 其后与路径 A 相同（ExitBootServices → init ELF …）
```

| 注意 | |
|------|--|
| 图里不要写成「一定是 GRUB 加载 **同时又** 固件直接加载 bzImage」同一步 | 二选一为主；组合部署存在，但是两条机制 |
| 嵌入式 | 常是 **U-Boot**（非 UEFI）或 ARM 上的 EFI；**「引导器/固件认一种格式，内核跑起来后认 ELF」** 的分界仍成立 |

---

## 五、常见误区

### 误区 1：GNU-EFI 编出来还是 ELF？

**错。** `gcc` 中间 `.o` 可以是 ELF；GNU-EFI / 链接脚本最终产出的 **`*.efi` 是 PE32+**。EDK2（TianoCore）产物同理。

### 误区 2：UEFI 能不能原生跑 ELF？

**标准规范不支持。** 迂回路线只有：

1. 自己写一个 **PE 格式** 的 UEFI 驱动，内嵌 ELF 解析器再加载 ELF；或  
2. 用 **GRUB 等引导器**（本身是 PE）代为解析 ELF/内核镜像。

### 误区 3：BSD 也是 ELF，为何不能直接被 UEFI 加载？

与 Linux 同构：内核 ELF → 要么 **EFI Stub 式 PE 包装**，要么 **`loader.efi`（PE）** 转发加载。

### 误区 4：`.ko` 是 ELF 吗？

**是** — 可重定位 ELF。  
软校正：用户态可执行文件走 **`fs/binfmt_elf.c`**；**模块**走 **内核模块加载器**（同样认 ELF，但不是 `execve` 那条 binfmt 路径）。二者都只在 **内核已运行** 后有意义，UEFI 阶段不认 `.ko`。

---

## 六、串到进程 / fork / exec（补全）

UEFI 阶段 **没有** Linux 进程模型：

| UEFI Boot Services 阶段 | 内核已接管之后 |
|-------------------------|----------------|
| 无 `task_struct` | 有进程描述符（Ch 3） |
| 无 `fork` / `execve` | `fork` 复制地址空间；`execve` **加载新 ELF** |
| 无用户态虚拟地址空间合同 | 有 mm / VMA（Ch 15） |
| 「程序」= 固件加载的 PE 镜像 | 「程序」= ELF；动态库也是 ELF |

启动后半段与 LKD 的衔接：

```
内核跑起来
  → 创建 1 号进程等内核线程 / init
  → execve("/sbin/init")     ← binfmt_elf 解析 ELF
  → init: fork + execve      ← 每个服务都是新的 ELF 映像
  → 驱动 insmod foo.ko       ← 模块加载器解析 ELF .ko
```

| 学习含义 | |
|----------|--|
| **Ch 2** | 你编出的 `bzImage`/`vmlinuz` 如何被固件或 GRUB 喂进 CPU |
| **Ch 3 / Ch 5** | 为何用户态「运行一个程序」几乎总是 **exec 一个 ELF** |
| **调试** | `readelf`/`file` 看用户态与 `vmlinux`；`*.efi` 用 PE 工具看，不要拿 `readelf` 硬套当唯一真相 |

快速自检：

```bash
file /bin/bash          # ELF
file /boot/vmlinuz-*    # 常显示 Linux kernel；带 stub 时工具可能提示 EFI/PE 相关
file /boot/efi/EFI/*/grubx64.efi   # PE32+ executable
```



<details>
<summary>自测题（点击展开）</summary>

**Q1.** UEFI 固件加载的是 PE32+ 格式还是 ELF？为什么内核却用 ELF？

<details><summary>答案</summary>

UEFI 固件规范要求 bootloader 是 PE32+ 格式（Windows 遗产）。Linux 内核通过 EFI stub 同时提供 PE 头让 UEFI 能加载它，加载后内核接管 CPU 就切换到 ELF 视角运行。EFI stub 让内核自身就是合法 EFI 应用，不需要 GRUB 中转。

</details>

**Q2.** 从按下电源到内核 start_kernel() 执行，经过了哪些阶段？

<details><summary>答案</summary>

1) UEFI 固件初始化硬件 → 2) UEFI 加载内核镜像（作为 PE32+ EFI 应用）→ 3) 内核 EFI stub 入口（arch/x86/boot/）→ 4) 解压内核（decompressor）→ 5) 进入保护模式/长模式 → 6) 跳转到 start_kernel()（init/main.c）。HFT 的 boot time 优化主要在减少固件 POST 和驱动初始化。

</details>

</details>
---

## 七、一页记忆卡

| 阶段 | 二进制世界 |
|------|------------|
| 固件还在管 Boot Services | **PE32+** |
| 内核已 ExitBootServices 并跑起来 | **ELF**（用户态 + 模块；`vmlinux` 调试镜像也是 ELF） |
| `EFI_STUB` | PE 外壳，好让固件肯加载内核 |
| GRUB `.efi` | PE；之后由它加载内核文件 |
| `fork`/`exec` | **只存在于 Linux 进程世界**，加载的是 ELF |

→ [§2.2 源码树](./section-2.2-内核源码树.md) · [§2.1](./section-2.1-获取内核源码.md) · [README](../../../../README.md)
