# §4.4 · CLI / STI / PUSHFD — palette critical section

| 文件 | 演示 |
|------|------|
| [bootpack.c](./bootpack.c) | **`io_cli()` → 设调色板 → `io_sti()`** |
| [asmfunc.asm](./asmfunc.asm) | **`io_cli` / `io_sti`** + **`palette_init_with_cli`**（`pushfd`…`popfd`） |

QEMU：**全屏绿色**（色号 6，在关中断临界区内写入调色板）。

笔记：[§4.4](../../notes/section-4.4-EFLAGS-寄存器与中断控制.md)
