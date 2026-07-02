# §4.1 · VRAM fill + stripes (QEMU / haribote)

Single-file **`HariMain`** demo: **full white** or **black/white stripes** via bit ops on **`0xA0000`**.

| File | Role |
|------|------|
| [bootpack.c](./bootpack.c) | **`fill_screen`** + **`draw_stripes_x_mask`** + **`draw_stripes_xor_cols`** |
| [asmfunc.asm](./asmfunc.asm) | **`io_hlt`** |

Notes: [§4.1](../../notes/section-4.1-用-C-写入显存与位运算.md)

---

## Depends on Day 3 boot chain

| File | From |
|------|------|
| `ipl.asm` / `ipl.bin` | [Day 3 sec-3.1-ipl-int13-disk-load](../../../day-03-32bit-c/code/sec-3.1-ipl-int13-disk-load/) |
| `nasmhead.asm` | [Day 3 sec-3.4-bootpack-asm-and-c](../../../day-03-32bit-c/code/sec-3.4-bootpack-asm-and-c/) — **sets mode 0x13** |
| `asmfunc.asm` | 本目录 |
| **`bootpack.c`** | 本目录 |

No `set_vga13h()` in C — nasmhead already did **`INT 0x10`**.

---

## Build bootpack (WSL / MSYS2)

```bash
cd day-04-c-graphics/code/sec-4.1-vram-fill-and-stripes

nasm -f elf32 ../../../day-03-32bit-c/code/sec-3.4-bootpack-asm-and-c/nasmhead.asm -o nasmhead.o
nasm -f elf32 asmfunc.asm -o asmfunc.o
gcc -m32 -c bootpack.c -o bootpack.o -ffreestanding -nostdlib -fno-pie

ld -m elf_i386 -Ttext 0x8200 -e start -o bootpack.bin \
   nasmhead.o bootpack.o asmfunc.o --oformat binary
```

Write **`ipl.bin`** @ 0 + **`bootpack.bin`** @ 512 into `haribote.img` — [Day 3 §3.1 README](../../../day-03-32bit-c/code/sec-3.1-ipl-int13-disk-load/README.md).

```powershell
D:\qemu\qemu-system-i386.exe -fda haribote.img -boot a
```

---

## Demo modes (edit `HariMain` bottom)

| Uncomment | QEMU screen |
|-----------|---------------|
| `fill_screen(..., PIXEL_WHITE)` only | **Full white** |
| `draw_stripes_x_mask` | **8-pixel vertical bars** (`x & 8`) |
| `draw_stripes_xor_cols` | **1-pixel vertical bars** (`i & 1`) |

**Colors:** `PIXEL_BLACK=0`, `PIXEL_WHITE=15` (BIOS default palette). Index **1** is not white on default VGA palette.

---

## vs DOS `char far *` tutorial

| DOS DJGPP | This file |
|-----------|-----------|
| `char far *vram = (char far *)0xA0000` | `volatile unsigned char *vram = (void *)0xA0000` |
| `set_vga13h()` in `main` | **nasmhead** before `call HariMain` |
| `getch()` exit | `io_hlt()` loop |
