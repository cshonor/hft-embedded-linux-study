# §4.2 · VRAM pointer walk (`*p` / `p++`)

| 文件 | 演示 |
|------|------|
| [bootpack.c](./bootpack.c) | **`char *p = 0xA0000`**，用 **`*p` + `p++`** 扫屏 — 左黑右白 |
| [asmfunc.asm](./asmfunc.asm) | 仅 **`io_hlt`** |

QEMU：**左半黑、右半白**（不用 `vram[y*320+x]`，体会指针即地址游标）。

笔记：[§4.2](../../notes/section-4.2-挑战并理解指针.md)

链接方式同 [§4.1](../sec-4.1-vram-fill-and-stripes/README.md)（nasmhead 用 Day 3，**bootpack.c 换本目录**）。
