# Ch15 `code/` 目录说明

TLPI 第 15 章（File Attributes）的可编译代码。分三类：

| 类别 | 文件 | 说明 |
|------|------|------|
| 自编 demo（6） | `c15_1_stat_family.c` `c15_2_timestamps.c` `c15_3_futimens.c` `c15_4_chown.c` `c15_5_chmod_umask.c` `c15_6_access.c` | 每个对应一节的实测主线，输出已钉进笔记 |
| 习题实现（5） | `ex15_1_perms.c` `ex15_3_nanosecond_stat.c` `ex15_4_eaccess.c` `ex15_5_umask_peek.c` `ex15_6_chmod_arX.c` | 15-2 为纸面题；15-7 Linux 专有给方案 |
| 原书镜像（8） | `t_stat.c`(Listing 15-1) `t_chown.c`(15-2) `file_perms.h`(15-3) `file_perms.c`(15-4) `t_umask.c`(15-5) `t_utime.c` `t_utimes.c` `chiflag.c`（补充程序） | dist 逐字保真 |
| 支撑（4） | `tlpi_hdr.h`（替身：errExit/fatal/usageErr/cmdLineErr/errMsg/Boolean） `ugid_functions.{h,c}`（dist 随附） `sys/sysmacros.h`（macOS 空壳 shim） | 让原书零改动编译 |

**实测环境**：macOS 26.6.2 (arm64) · clang 23.1.0（`cdev` 环境）。自编 11 个 + 原书 6 个可跑程序全部编译零警告、运行成功；`chiflag.c` 依赖 `<linux/fs.h>` 在 macOS 不可编译（Linux 专有，保留镜像，复测平台 Pi5/ext4）。

```bash
CC=clang
$CC -D_DARWIN_C_SOURCE -I. -Wall -Wextra -o c15_1_stat_family c15_1_stat_family.c && ./c15_1_stat_family
$CC -D_DARWIN_C_SOURCE -I. -Wall -Wextra -o c15_2_timestamps c15_2_timestamps.c && ./c15_2_timestamps
$CC -D_DARWIN_C_SOURCE -I. -Wall -Wextra -o c15_3_futimens c15_3_futimens.c && ./c15_3_futimens
$CC -D_DARWIN_C_SOURCE -I. -Wall -Wextra -o c15_4_chown c15_4_chown.c && ./c15_4_chown
$CC -D_DARWIN_C_SOURCE -I. -Wall -Wextra -o c15_5_chmod_umask c15_5_chmod_umask.c && ./c15_5_chmod_umask
$CC -D_DARWIN_C_SOURCE -I. -Wall -Wextra -o c15_6_access c15_6_access.c && ./c15_6_access

# 原书（shim 使其零改动编译）
$CC -D_DARWIN_C_SOURCE -I. -Wall -o t_stat t_stat.c file_perms.c && ./t_stat /etc/hosts && ./t_stat -l /etc/hosts
$CC -D_DARWIN_C_SOURCE -I. -Wall -o t_umask t_umask.c file_perms.c && ./t_umask
$CC -D_DARWIN_C_SOURCE -I. -Wall -o t_utime t_utime.c && ./t_utime /etc/hosts
$CC -D_DARWIN_C_SOURCE -I. -Wall -o t_utimes t_utimes.c && ./t_utimes /etc/hosts
$CC -D_DARWIN_C_SOURCE -I. -Wall -o t_chown t_chown.c ugid_functions.c && ./t_chown - - /etc/hosts
```

> ⚠️ `t_umask` 会在**当前目录**创建/删除 `myfile`/`mydir`，请在临时目录里跑。
> ⚠️ `c15_4_chown` 的改组实验会把测试文件属组改到 wheel 再改回来——全部在 `/tmp` 的一次性文件上操作，不碰既有文件。
> Linux 复测（Pi5）：`gcc -Wall -o chiflag chiflag.c && sudo ./chiflag +a /tmp/x`（i-node flags 需要 ext4 + 特权）。
