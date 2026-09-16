# Ch16 `code/` 目录说明

TLPI 第 16 章（Extended Attributes）的可编译代码。

| 类别 | 文件 | 说明 |
|------|------|------|
| 自编 demo | `c16_1_xattr_basic.c` | set/get/list/remove 四件套 + 探大小 + 二进制值 + ERANGE + ENOATTR/ENODATA |
| | `c16_2_namespaces.c` | namespace 试探 + symlink 限制 + fsetxattr + 写权限语义 |
| 习题 | `ex16_1_setfattr.c` | 16-1 简易 setfattr(1)（user.* 限定 + 回读验证） |
| 原书镜像 | `xattr_view.c`(Listing 16-1) `t_setxattr.c`(补充) | dist 逐字；`t_setxattr` 为 5 参 Linux 签名，仅 Linux 可编 |
| 支撑 | `tlpi_hdr.h` 替身 | 头注释标注两平台签名差异 |

**实测环境**：macOS 26.6.2 (arm64) · clang 23.1.0。自编 3 个程序编译零警告、运行成功；xattr 需要 FS 支持（APFS 支持 user.*；系统目录/特殊文件可能不行，demo 全部在 /tmp 的一次性文件上做）。

```bash
CC=clang
$CC -Wall -Wextra -o c16_1_xattr_basic c16_1_xattr_basic.c && ./c16_1_xattr_basic
$CC -Wall -Wextra -o c16_2_namespaces c16_2_namespaces.c && ./c16_2_namespaces
touch /tmp/x16.txt
$CC -Wall -Wextra -o ex16_1_setfattr ex16_1_setfattr.c && ./ex16_1_setfattr /tmp/x16.txt origin tokyo
```

> macOS 的 xattr 系列多 `position/options` 两参；demo 内用移植宏抹平，Linux（Pi5）上同一份代码直接编。
> 验证工具：`xattr -l /tmp/x16.txt`（macOS）或 `getfattr -d`（Linux）可与 demo 输出对拍。
