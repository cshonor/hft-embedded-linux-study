# §3.1 · IPL — INT 0x13 disk load

| File | Role |
|------|------|
| [ipl.asm](./ipl.asm) | **512 B** boot sector — read 10 cylinders into `0x8200`, then `JMP 0x8200` |
| [ipl.bin](./ipl.bin) | NASM output (`nasm -f bin`) |

Notes: [§3.1 index](../../notes/section-3.1-制作真正的-IPL-与读取磁盘.md) · [§3.1.3 code walkthrough](../../notes/section-3.1.3-INT0x13与ipl代码拆解.md)

---

## Build `ipl.bin` (512 bytes)

```powershell
nasm -f bin ipl.asm -o ipl.bin
```

Check: file size **512**; last two bytes **`55 AA`**.

---

## Write to floppy image (same as Day 2)

**PowerShell:**

```powershell
$size = 1474560
$disk = New-Object byte[] $size
$ipl  = [IO.File]::ReadAllBytes("$PWD\ipl.bin")
[Array]::Copy($ipl, 0, $disk, 0, 512)
[IO.File]::WriteAllBytes("$PWD\haribote.img", $disk)
```

> IPL alone shows **load done** then jumps to empty memory at `0x8200` until you also build and write **bootpack** from [sec-3.4-bootpack-asm-and-c](../sec-3.4-bootpack-asm-and-c/).

---

## QEMU

```powershell
D:\qemu\qemu-system-i386.exe -fda haribote.img -boot a
```
