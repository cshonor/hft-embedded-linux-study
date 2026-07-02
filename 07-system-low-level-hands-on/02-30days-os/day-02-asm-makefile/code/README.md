# Day 2 · 启动区 IPL + Makefile

| 文件 | 说明 |
|------|------|
| [helloos.asm](./helloos.asm) | 启动区 **源码**（文本） |
| [ipl.bin](./ipl.bin) | **512 B** 引导扇区（NASM 产物） |
| [helloos.img](./helloos.img) | **1,474,560 B** 软盘镜像（`ipl.bin` 嵌在偏移 0） |
| [Makefile](./Makefile) | Linux/macOS：`make ipl` · `make img` · `make run` |

---

## 命令一览（无封装脚本 · 可直接复用）

在 `day-02-asm-makefile/code/` 目录下执行。

### ① 汇编 → `ipl.bin`（512 B）

```bash
nasm -f bin helloos.asm -o ipl.bin
```

### ② 拼盘 → `helloos.img`（1,474,560 B）

**Linux / macOS / MSYS2：**

```bash
dd if=/dev/zero of=helloos.img bs=512 count=2880
dd if=ipl.bin of=helloos.img conv=notrunc
```

**Windows PowerShell（等价于上面两条 `dd`）：**

```powershell
$size = 1474560
$disk = New-Object byte[] $size
$ipl  = [IO.File]::ReadAllBytes("$PWD\ipl.bin")
[Array]::Copy($ipl, 0, $disk, 0, 512)
[IO.File]::WriteAllBytes("$PWD\helloos.img", $disk)
```

**HxD 手工：** 新建 **1,474,560 B** → 把 **`ipl.bin` 整文件粘贴到偏移 0**（[Day 1 §1.4](../../day-01-boot-asm/notes/section-1.4-加工润色.md)）。

### ③ QEMU 启动

**PATH 里已有 `qemu-system-i386` 时：**

```bash
qemu-system-i386 -fda helloos.img -boot a
```

**Windows · QEMU 装在 `D:\qemu`（与 [Day 1 §1.1.5](../../day-01-boot-asm/notes/section-1.1.5-QEMU安装与运行.md) 相同）：**

```powershell
cd <本仓库>\07-system-low-level-hands-on\02-30days-os\day-02-asm-makefile\code
D:\qemu\qemu-system-i386.exe -fda helloos.img -boot a
```

或先把映像拷到工作目录再启动（Day 1 习惯）：

```powershell
Copy-Item helloos.img D:\haribote\boot.img
cd D:\haribote
D:\qemu\qemu-system-i386.exe -fda boot.img -boot a
```

> **勿**把 `helloos.img` / `boot.img` 放在 `D:\qemu\` 安装目录内。

---

## 命名：`ipl.bin` 与 `helloos.img`

| 文件 | 大小 | 含义 |
|------|------|------|
| **`ipl.bin`** | 512 B | **IPL** 启动区裸二进制；**不是** QEMU 直接 `-fda` 的对象 |
| **`helloos.img`** | 1,474,560 B | 1.44 MB 软盘镜像；**偏移 0 的 512 B = `ipl.bin` 全文** |
| **`boot.img`** | 同上 | 与 `helloos.img` **同物**，Day 1 常用文件名 |

**QEMU 用 `helloos.img`，不用 `ipl.bin`** — 模拟器需要 **2880 扇区** 整盘，不能只给 512 B。

→ 详解：[section-2.3 · 命名与拼盘](../notes/section-2.3-先制作启动区.md)

---

## 自检

| 文件 | 检查 |
|------|------|
| **`ipl.bin`** | 512 B；HxD 偏移 **`0x1FE`** = **`55 AA`** |
| **`helloos.img`** | 1,474,560 B；**`0x1FE`** = **`55 AA`** |

---

[section-2.4 · Makefile](../notes/section-2.4-Makefile-入门.md) · [Day 1 §1.3 NASM](../../day-01-boot-asm/notes/section-1.3-初次体验汇编程序.md)
