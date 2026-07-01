# Day 3 · 真正 IPL（读盘）

| 文件 | 说明 |
|------|------|
| [ipl.asm](./ipl.asm) | **512 B** 启动扇区源码（`INT 0x13` 读 10 柱面 → `0x8200`） |
| [ipl.bin](./ipl.bin) | NASM 产物（`make ipl` 或下方命令生成） |

笔记：[§3.1 制作真正的 IPL](../notes/section-3.1-制作真正的-IPL-与读取磁盘.md)

---

## ① 汇编 → `ipl.bin`（512 B）

```bash
nasm -f bin ipl.asm -o ipl.bin
```

PowerShell 同目录：

```powershell
nasm -f bin ipl.asm -o ipl.bin
```

自检：文件大小 **512**；末尾两字节 **`55 AA`**（可用 Day 1 的 HxD 或 `Format-Hex` 查看）。

---

## ② 拼进 1.44 MB 软盘（与 Day 2 相同）

**Linux / macOS / MSYS2：**

```bash
dd if=/dev/zero of=haribote.img bs=512 count=2880
dd if=ipl.bin of=haribote.img conv=notrunc
```

**Windows PowerShell：**

```powershell
$size = 1474560
$disk = New-Object byte[] $size
$ipl  = [IO.File]::ReadAllBytes("$PWD\ipl.bin")
[Array]::Copy($ipl, 0, $disk, 0, 512)
[IO.File]::WriteAllBytes("$PWD\haribote.img", $disk)
```

> 仅 IPL、尚未写入 **bootpack** 时，QEMU 会显示 **load done** 后 **`JMP 0x8200`** 到空内存 — 正常；完整 haribote-os 需 [§3.2](../notes/section-3.2-纸娃娃操作系统.md) 起的 **nasmhead + bootpack** 一并写入软盘（原书工具链或后续 Day 的 Makefile）。

---

## ③ QEMU 启动

```bash
qemu-system-i386 -fda haribote.img -boot a
```

**Windows · QEMU 在 `D:\qemu`：**

```powershell
D:\qemu\qemu-system-i386.exe -fda haribote.img -boot a
```

---

## Makefile（可选 · Linux / MSYS2）

```makefile
NASM = nasm
NASMFLAGS = -f bin

ipl.bin: ipl.asm
	$(NASM) $(NASMFLAGS) $< -o $@

clean:
	rm -f ipl.bin haribote.img
```

Windows 本机以 **上方明文命令** 为准（同 [Day 2 README](../../day-02-asm-makefile/code/README.md)）。
