## 写入引导扇区机器码

**对照资料：** [HELLOOS_HEX_REFERENCE.md](../../HELLOOS_HEX_REFERENCE.md) · [helloos-boot-sector-paste.txt](../code/helloos-boot-sector-paste.txt)（**HxD 粘贴用**）· [helloos-boot-sector.hex](../code/helloos-boot-sector.hex)（逐行手输对照）· [helloos.img](../code/helloos.img)

> **勿从 `.hex` 整段复制：** 每行左侧 `0000`、`0100`… 是地址标尺（同 HxD 左列），粘进映像会导致错位。整段粘贴请用 **[helloos-boot-sector-paste.txt](../code/helloos-boot-sector-paste.txt)**。

从 **中间 hex 区偏移 0 的第一个格子** 起粘贴或对照输入；也可一键复制参考 `helloos.img`（见 [1.1.6 做法 A](./section-1.1.6-启动链路与排错.md)）。

### 极简改法（建立直觉）

文件已是 **1,474,560 B**、其余全是 `00` 时，**关键两处**（中间大量 `00` **不用动**）：

| 位置 | 怎么找 | 改什么 |
|------|--------|--------|
| **引导扇区开头** | 第一行 **`00` 列**（偏移 **0**） | **`EB` `4E` `90`** …（第 2 字节是 **`4E`**，不是 `FE`） |
| **引导扇区末尾** | **`Ctrl+G` → `1FE`** | **`55` `AA`**（第 512 字节末两格，**不是** 全盘物理末尾） |

只改开头 + `55 AA` **不够** 打出 `hello, world` — 还需 **`0x050`–`0x083`** 的 **`INT 0x10` 循环** 与字符串。完整 512 B 见 hex 对照表。

### 偏移速查

| 偏移 | 十六进制 | 含义 |
|------|----------|------|
| `0x000` | `EB 4E 90` | 短跳转 + NOP，跳过 FAT12 参数区 |
| `0x003` 起 | `HELLOIPL` | OEM 名（8 字节），非屏幕显示 |
| `0x050` 起 | `B8 00 00 8E D0 …` | 初始化段寄存器、`INT 0x10` 打印 |
| `0x076` 起 | `hello, world` | 显示字符串 |
| **`0x1FE`** | **`55 AA`** | 引导扇区签名 |

`EB 4E 90` 后是 **磁盘格式参数**；屏幕上 **`hello, world`** 在数据区，由 **`INT 0x10`** 打出。

← [1.1.2 新建映像](./section-1.1.2-HxD界面与新建映像.md) · 下一步 [1.1.4 启动签名与自检](./section-1.1.4-启动签名与自检.md)
