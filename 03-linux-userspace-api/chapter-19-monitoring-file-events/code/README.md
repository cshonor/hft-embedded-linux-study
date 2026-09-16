# Ch19 代码 — Monitoring File Events（inotify / dnotify）

> ⚠️ **全部 Linux 专有**（inotify 需 2.6.13+）。macOS 无 `sys/inotify.h`，本目录程序**未在本机编译运行**；
> 源码按 man-pages 6.19 `inotify(7)` 与 Linux v6.6 `fs/notify/inotify/` 核验，Pi5 复测步骤见[章 README](../README.md)。

## 编译（Linux / Pi5）

```bash
CC=cc
# 自编 + 习题（无额外依赖）
$CC -Wall -Wextra -o inotify_dir       inotify_dir.c
$CC -Wall -Wextra -o inotify_epoll     inotify_epoll.c
$CC -Wall -Wextra -o ex19_1_log_events ex19_1_log_events.c

# 原书镜像（需 tlpi_hdr.h 替身；dnotify 需 _GNU_SOURCE 取 DN_* 常量）
D=-D_GNU_SOURCE
$CC $D -I. -Wall -Wextra -o demo_inotify  demo_inotify.c
$CC $D -I. -Wall -Wextra -o dnotify       dnotify.c
$CC $D -I. -Wall -Wextra -o inotify_dtree inotify_dtree.c
$CC $D -I. -Wall -Wextra -o rand_dtree    rand_dtree.c
```

## 文件清单

### 自编

| 文件 | 节 | 要点 |
|------|----|------|
| `inotify_dir.c` | 19.4 | 形态 A：阻塞 `read` 监控单目录；变长记录步进 `sizeof(inotify_event) + ev->len` |
| `inotify_epoll.c` | 19.4 | 形态 B：`IN_NONBLOCK` + `epoll` 事件循环（Ch63 预演）；读到 `EAGAIN` 为止 |
| `ex19_1_log_events.c` | 19.8 | 习题 19-1：`nftw(FTW_PHYS)` 枚举挂 watch；`IN_CREATE\|IN_ISDIR`/`IN_MOVED_TO\|IN_ISDIR` 动态补 watch + 立即 rescan 补漏；`IN_IGNORED`/`IN_DELETE_SELF` 清映射；`IN_Q_OVERFLOW` 告警 |

### 原书镜像（TLPI dist `inotify/`，逐字）

| 文件 | 出处 | 说明 |
|------|------|------|
| `demo_inotify.c` | **Listing 19-1** | 监控命令行给定路径的全部事件并打印——API 速查的活字典 |
| `dnotify.c` | 官方补充 | dnotify 用法 `dnotify dir1:a xyz/dir2:acdM`；`F_SETSIG` + realtime 信号 + `si_fd` 三件套 |
| `inotify_dtree.c` | 官方补充 | **健用法参考实现**：维护目录树缓存，与文件系统保持一致（rename 配对、溢出重建） |
| `rand_dtree.c` | 官方补充 | 随机 mkdir/rmdir/rename 子目录，配合 `inotify_dtree` 做正确性压测 |

### 支撑

| 文件 | 说明 |
|------|------|
| `tlpi_hdr.h` | 官方 `lib/tlpi_hdr.h` 的最小替身（`errExit`/`fatal`/`usageErr`/`cmdLineErr` + `get_num.h`），头注释写明与真实实现的差异 |

## 典型运行（Pi5）

```bash
mkdir -p /tmp/tlpi_inotify
./inotify_dir /tmp/tlpi_inotify
# 另一终端：
#   touch /tmp/tlpi_inotify/a; echo x >> /tmp/tlpi_inotify/a; rm /tmp/tlpi_inotify/a
#   mv /tmp/tlpi_inotify/x /tmp/tlpi_inotify/y   ← 观察 IN_MOVED_FROM/TO 的 cookie 配对

./ex19_1_log_events /tmp/tree &     # 子树监控；另一终端 mkdir/rmdir/mv 看 watch 集合变化
```

> `inotify_dtree` + `rand_dtree` 的组合：两个程序并行长跑，`rand_dtree` 随机扰动、
> `inotify_dtree` 用事件维护缓存并自校验——是「inotify 状态一致性」的最强参考实现。
