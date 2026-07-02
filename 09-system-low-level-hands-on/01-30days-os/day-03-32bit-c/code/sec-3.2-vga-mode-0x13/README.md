# §3.2 · VGA mode 0x13 (320×200×256-color index)

Mode switch is **not** a separate file — it lives in **nasmhead.asm** (must run in **16-bit** before protected mode):

```nasm
        mov     ax, 0x0013        ; BIOS mode 13h: 320×200, 8bpp palette index
        int     0x10
```

| Item | Value |
|------|-------|
| Frame buffer | **`0xA0000`** … **`0xA0000 + 63999`** |
| Pixel | **1 byte** = palette index **0–255** |
| Addressing | **`offset = y * 320 + x`** |

Restore text mode (debug only): `mov ax, 0x0003` + `int 0x10`.

**vs text mode 0x03:** VRAM **`0xB8000`**, 2 bytes/char — see [§3.2.1](../../notes/section-3.2.1-VGA模式0x13详解.md).

Full entry: [nasmhead.asm](../sec-3.4-bootpack-asm-and-c/nasmhead.asm)  
Black-screen fill: [bootpack.c](../sec-3.4-bootpack-asm-and-c/bootpack.c)  
Day 4 white/stripes: [bootpack-day4.c](../../../day-04-c-graphics/code/sec-4.1-vram-fill-and-stripes/bootpack-day4.c)

Notes: [§3.2](../../notes/section-3.2-纸娃娃操作系统.md) · [§3.2.1](../../notes/section-3.2.1-VGA模式0x13详解.md)
