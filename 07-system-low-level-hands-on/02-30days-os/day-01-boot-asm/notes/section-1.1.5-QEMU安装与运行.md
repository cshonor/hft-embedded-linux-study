## QEMU 安装与运行

**不必装 VMware / VirtualBox** — Day 1 只需 **QEMU** 模拟 **x86 PC 从软盘 A: 引导**。

### 获取（官方 `qemu-w64-setup`）

1. [qemu.org/download#windows](https://www.qemu.org/download/#windows) → **Windows** 标签

![QEMU 官网 Windows 下载页](../../assets/qemu-org-download-windows.png)

2. **Stefan Weil provides binaries…** → **`64-bit`** → [qemu.weilnetz.de/w64/](https://qemu.weilnetz.de/w64/)

![QEMU 64-bit 镜像站](../../assets/qemu-weilnetz-w64.png)

3. 下载最新 **`qemu-w64-setup-YYYYMMDD.exe`**（约 190 MB）。**跳过 MSYS2 / pacman** 段落。

**不要**用来源不明的「QEMU 便携版」第三方打包。

### 安装

双击 setup → **Choose Install Location** → 建议 **`D:\qemu`**（约 1.1 GB）→ 勾选 **Add to PATH**（若有）→ **Install**。

![QEMU 安装路径](../../assets/qemu-install-path.png)

新开 cmd：`qemu-system-i386 --version`

### 运行 boot.img

1. HxD **`Ctrl+S`**；确认 **1440 KB**（`1474560` B）
2. PowerShell（在 `D:\qemu` 目录，或写完整路径）：

```cmd
D:\qemu\qemu-system-i386.exe -fda D:\haribote\boot.img -boot a
```

| 参数 | 含义 |
|------|------|
| **`-fda`** | 把映像挂到虚拟 **A: 软驱**（软盘），**不是**硬盘 |
| **`-boot a`** | BIOS **先从软盘 A 启动**（推荐 Day 1 加上，避免先报硬盘失败） |

**成功标志：** 出现 **`Booting from Floppy...`** 后打印 **`hello, world`**。

```
Booting from Floppy...
hello, world
```

![QEMU `-boot a` 软盘引导成功](../../assets/qemu-boot-a-hello-world-success.png)

> **不加 `-boot a` 时：** BIOS 默认 **先试硬盘 (C:)**，会先出现 `Boot failed: could not read the boot disk`，再试软盘 —— **不是失败**，只是多两行提示。加 **`-boot a`** 可跳过硬盘那一步。

`WARNING: image format was not specified` 可忽略。关窗口用 **`Ctrl+Alt+G`** 释放鼠标。

← [1.1.4 启动签名](./section-1.1.4-启动签名与自检.md) · 下一步 [1.1.6 排错](./section-1.1.6-启动链路与排错.md)
