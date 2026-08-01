# Day 1 · helloos 映像参考文件

> 与 [Day 1 笔记](../) 同目录 · 非 tolset 版权资源，仅为学习对照用字节表

| 文件 | 说明 |
|------|------|
| [helloos.img](./helloos.img) | 完整 **1,474,560 B** 软盘映像，可直接 `qemu-system-i386 -fda helloos.img` |
| [helloos-boot-sector.bin](./helloos-boot-sector.bin) | 仅引导扇区 **512 B** |
| [helloos-boot-sector.hex](./helloos-boot-sector.hex) | 512 字节 · 16 字节/行，**逐行对照手输**用（行首偏移勿复制） |
| **[helloos-boot-sector-paste.txt](./helloos-boot-sector-paste.txt)** | **HxD 整段粘贴首选** · 512 字节 · 无地址前缀 |

完整进制说明见 [HELLOOS_HEX_REFERENCE.md](../../HELLOOS_HEX_REFERENCE.md)。

**HxD 用法（引导扇区）：**

1. 新建/打开 `boot.img` → `Ctrl+E` → `1474560`
2. **`Ctrl+G` → `0`** → 打开 **[helloos-boot-sector-paste.txt](./helloos-boot-sector-paste.txt)** 全选复制粘贴（**不要**从 `.hex` 复制，行首 `0000`/`0100` 是地址标尺）
3. 或逐行对照 [helloos-boot-sector.hex](./helloos-boot-sector.hex) 手输（每行只输入空格后的 16 字节）
4. `Ctrl+G` → `1FE` → 确认 `55 AA`
5. 或复制本目录 **`helloos.img`** 到 `D:\haribote\boot.img` 再 QEMU 启动
