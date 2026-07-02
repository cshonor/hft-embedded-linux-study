# Day 4 · Code — one example per section

Each folder is a **`HariMain` + `asmfunc`** pair. Link with Day 3 **IPL + nasmhead** (see [§4.1 build](./sec-4.1-vram-fill-and-stripes/README.md)).

| Folder | § | QEMU 现象 |
|--------|---|-----------|
| [sec-4.1-vram-fill-and-stripes](./sec-4.1-vram-fill-and-stripes/) | 4.1 | 全白 / 黑白条纹（`for` + 位运算） |
| [sec-4.2-vram-pointer-walk](./sec-4.2-vram-pointer-walk/) | 4.2 | **左黑右白**（`*p` + `p++`） |
| [sec-4.3-palette-16-colors](./sec-4.3-palette-16-colors/) | 4.3 | **黑 \| 红 \| 亮灰** 三区（调色板 + OUT） |
| [sec-4.4-cli-sti-palette](./sec-4.4-cli-sti-palette/) | 4.4 | **全屏绿**（CLI/STI 设调色板） |
| [sec-4.5-boxfill-taskbar](./sec-4.5-boxfill-taskbar/) | 4.5 | **白底 + 底栏黑条**（`boxfill`） |

Boot chain（各节共用）：

| 文件 | 来源 |
|------|------|
| `ipl.asm` / `ipl.bin` | [Day 3 §3.1](../../day-03-32bit-c/code/sec-3.1-ipl-int13-disk-load/) |
| `nasmhead.asm` | [Day 3 §3.4](../../day-03-32bit-c/code/sec-3.4-bootpack-asm-and-c/) |
| **`bootpack.c`** | **本表某一节的 `bootpack.c`** |
| **`asmfunc.asm`** | **同节目录**（§4.1/4.2/4.5 仅 `io_hlt`；§4.3/4.4 含调色板 asm） |

## Build bootpack（WSL / MSYS2，在某一节目录下）

```bash
NASMHEAD=../../../day-03-32bit-c/code/sec-3.4-bootpack-asm-and-c/nasmhead.asm

nasm -f elf32 $NASMHEAD -o nasmhead.o
nasm -f elf32 asmfunc.asm -o asmfunc.o
gcc -m32 -c bootpack.c -o bootpack.o -ffreestanding -nostdlib -fno-pie
ld -m elf_i386 -Ttext 0x8200 -e start -o bootpack.bin \
   nasmhead.o bootpack.o asmfunc.o --oformat binary
```

拼软盘： [Day 3 §3.1 README](../../day-03-32bit-c/code/sec-3.1-ipl-int13-disk-load/README.md) · QEMU `-fda haribote.img -boot a`
