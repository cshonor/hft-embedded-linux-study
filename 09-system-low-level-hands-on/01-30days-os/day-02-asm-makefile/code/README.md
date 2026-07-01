# Day 2 · 启动区 IPL + Makefile

| 文件 | 说明 |
|------|------|
| [helloos.asm](./helloos.asm) | 启动区 **源码**（文本） |
| [ipl.bin](./ipl.bin) | **512 B** 引导扇区（NASM 产物，已提交可对照） |
| [helloos.img](./helloos.img) | **1,474,560 B** 软盘镜像（`ipl.bin` 嵌在偏移 0，QEMU 用） |
| [Makefile](./Makefile) | `make ipl` · `make img`（Linux/macOS） |
| [build-img.ps1](./build-img.ps1) | Windows 拼盘脚本 |

---

## 三步与命名

| 步 | 命令 | 产出 | 谁用 |
|----|------|------|------|
| ① 汇编 | `nasm -f bin helloos.asm -o ipl.bin` | **`ipl.bin`** 512 B | CPU（经 BIOS 加载） |
| ② 拼盘 | 见下 [ipl.bin 拼成 helloos.img](#iplbin-拼成-helloosimg) | **`helloos.img`** 1.44 MB | **QEMU / 虚拟机** |
| ③ 启动 | `qemu-system-i386 -fda helloos.img -boot a` | 屏幕 `hello, world` | 你 |

**命名对照：**

| 名字 | 含义 |
|------|------|
| **`ipl.bin`** | **IPL** = Initial Program Loader；**`.bin`** = 裸二进制，**只有启动区** |
| **`helloos.img`** | **`.img`** = 整盘磁盘镜像；**不是** 把 `.bin` 改后缀，而是 **512 B 贴进 1.44 MB 空盘开头** |
| **`boot.img`** | Day 1 常用别名，**与 `helloos.img` 同物**（复制到 `D:\haribote\boot.img` 仅为路径习惯） |

**QEMU 必须 `-fda helloos.img`，不能 `-fda ipl.bin`** — 模拟器要 **2880 扇区** 的软盘，不是 512 B 单文件。

---

## 用法

```bash
cd day-02-asm-makefile/code
nasm -f bin helloos.asm -o ipl.bin    # 或: make ipl
```

### `ipl.bin` 拼成 `helloos.img`

**Windows（PowerShell）：**

```powershell
.\build-img.ps1
```

**Linux / macOS：**

```bash
dd if=/dev/zero of=helloos.img bs=512 count=2880
dd if=ipl.bin of=helloos.img conv=notrunc
# 或: make img
```

**HxD 手工：** 新建 **1,474,560 B** 文件 → 把 **`ipl.bin` 全部复制到偏移 0**（与 [Day 1 §1.4](../../day-01-boot-asm/notes/section-1.4-加工润色.md) 相同）。

### QEMU 启动

```bash
qemu-system-i386 -fda helloos.img -boot a
```

---

## 自检

| 文件 | 检查 |
|------|------|
| **`ipl.bin`** | 大小 **512**；HxD **`0x1FE`** = **`55 AA`** |
| **`helloos.img`** | 大小 **1,474,560**；**`0x1FE`** = **`55 AA`**；含字符串 `hello, world` |

---

完整说明：[section-2.3 · 启动区与命名](../notes/section-2.3-先制作启动区.md) · [section-2.4 · Makefile](../notes/section-2.4-Makefile-入门.md)
