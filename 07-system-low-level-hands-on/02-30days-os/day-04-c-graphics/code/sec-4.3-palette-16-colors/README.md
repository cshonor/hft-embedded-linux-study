# §4.3 · Palette — OUT 0x3c8 / 0x3c9

| 文件 | 演示 |
|------|------|
| [bootpack.c](./bootpack.c) | 设调色板后写 VRAM **0 / 7 / 15** → 黑 / 红 / 灰白 三区 |
| [asmfunc.asm](./asmfunc.asm) | **`set_palette_rgb`** — C 调汇编 **`OUT`** 写端口 |

QEMU：横条 **黑 | 红 | 亮灰**（证明 **VRAM 字节是索引**，颜色由调色板决定）。

笔记：[§4.3](../../notes/section-4.3-调色板Palette与色号.md)
