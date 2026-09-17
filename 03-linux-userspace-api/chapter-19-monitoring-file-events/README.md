# TLPI 第 19 章 — Monitoring File Events

**优先级**：🔴（热重载、配置监视、构建工具、日志跟随——配置热重载与落盘感知的根基）
**前置**：[Ch18 Directories and Links](../chapter-18-directories-links/README.md)（safe-write 与 nftw）· [Ch04 File I/O](../chapter-04-file-io-universal/README.md)（read 语义）
**后置**：[Ch20 Signals](../chapter-20-signals-fundamentals/README.md) · [Ch63 替代 I/O 模型](../chapter-63-alternative-i-o-models/notes/63.4-the-epoll-api.md)（inotify fd 进 epoll）

> ⚠️ **本章是 Linux 专有章**：inotify（2.6.13+）与 dnotify 在 macOS 上都不存在（macOS 等价物是 kqueue/FSEvents，语义不对齐）。
> **本轮实测环境不覆盖本章**——Pi（192.168.31.109）离线，本机无 docker/VM。全部笔记按 **man-pages 6.19** `inotify(7)`/`inotify_init(2)`/`inotify_add_watch(2)`/`fcntl(2)`(F_NOTIFY) + **Linux v6.6** `fs/notify/inotify/`（含 `IN_ONLYDIR`→`ENOTDIR` 这类错误码级的源码核验）撰写，所有程序标注「未实测」，Pi5 复测清单见下。
> 节号与官方目录 `toc-detailed.html` 逐条对齐（19.1–19.8，无子节）。

---

## 小节目录

- [19.1 Overview 概述（三代技术）](notes/19.1-overview.md)
- [19.2 The inotify API](notes/19.2-the-inotify-api.md)
- [19.3 inotify Events](notes/19.3-inotify-events.md)
- [19.4 Reading inotify Events](notes/19.4-reading-inotify-events.md)
- [19.5 Queue Limits and /proc Files](notes/19.5-queue-limits-and-proc-files.md)
- [19.6 An Older System: dnotify](notes/19.6-an-older-system-for-monitoring-file-even.md)
- [19.7 Summary](notes/19.7-summary.md)
- [19.8 Exercise](notes/19.8-exercise.md)

---

## 章节目标

- **分清三代技术**：轮询（正确但贵）→ dnotify（信号通知、不带文件名、fd 钉目录——已淘汰）→ inotify（fd 化事件队列：可 read、可 poll/epoll、带文件名与改名配对）
- **API 只有三个 syscall，难在一致性**：watch 挂 **inode** 不挂路径（改名不丢监控、safe-write 替换的文件必失效——所以要监控**目录**）；watch **不递归**（`IN_CREATE|IN_ISDIR`/`IN_MOVED_TO|IN_ISDIR` 时补 watch 并立即 rescan）
- **事件流的三个工程事实**：变长记录（步进 `sizeof + len`，缓冲保底 `sizeof + NAME_MAX + 1`）；`cookie` 只在 `MOVED_FROM/TO` 配对（跨 FS 移动配不上——把改名降级成「删+建」永远安全）；`IN_Q_OVERFLOW` = 丢事件 = **必须全量重建**
- **容量面**：`max_queued_events`（溢出静默丢）/ `max_user_instances`（每真实用户）/ `max_user_watches`（`ENOSPC`）；队列满的第一根因是「事件循环做重活」——重活必须异步
- **诚实**：本章含一处错误码级源码核验——`IN_ONLYDIR` 对非目录报 **`ENOTDIR`**（内核 `fs/notify/inotify/inotify_user.c:756` 经 `LOOKUP_DIRECTORY` 让路径解析层拒绝），man-pages 未明写；其余预期输出按语义推演并明确标注「非实测」

---

## 代码示例

| 文件 | 性质 | 覆盖 | 节 |
|------|------|------|----|
| [`inotify_dir.c`](code/inotify_dir.c) | 自编 | 形态 A：阻塞 read 监控单目录 | 19.4 |
| [`inotify_epoll.c`](code/inotify_epoll.c) | 自编 | 形态 B：非阻塞 + epoll 事件循环 | 19.4 |
| [`ex19_1_log_events.c`](code/ex19_1_log_events.c) | 习题 19-1 | nftw 枚举 + 动态增删 watch 的递归日志器 | 19.8 |
| [`demo_inotify.c`](code/demo_inotify.c) | **Listing 19-1** | 原书全事件演示 | 19.2 |
| [`dnotify.c`](code/dnotify.c) | 官方补充 | dnotify + `F_SETSIG` realtime 信号三件套 | 19.6 |
| [`inotify_dtree.c`](code/inotify_dtree.c) | 官方补充 | **健用法参考实现**：目录树缓存 + 事件一致性维护 | 19.7 |
| [`rand_dtree.c`](code/rand_dtree.c) | 官方补充 | 随机增删改子目录，压测 `inotify_dtree` | 19.7 |
| [`tlpi_hdr.h`](code/tlpi_hdr.h) | 支撑 | 官方头文件的 macOS 替身（含 get_num.h） | — |

**编译**（仅 Linux/Pi5）：

```bash
cc -Wall -Wextra -o inotify_dir    inotify_dir.c
cc -Wall -Wextra -o inotify_epoll  inotify_epoll.c
cc -Wall -Wextra -o ex19_1_log_events ex19_1_log_events.c
D=-D_GNU_SOURCE   # dnotify 需要 DN_* 常量
cc $D -I. -Wall -Wextra -o demo_inotify  demo_inotify.c
cc $D -I. -Wall -Wextra -o dnotify       dnotify.c
cc $D -I. -Wall -Wextra -o inotify_dtree inotify_dtree.c
cc $D -I. -Wall -Wextra -o rand_dtree    rand_dtree.c
```

---

## 六条纪律（全章的工程兑现）

1. **watch 不递归** → `IN_CREATE|IN_ISDIR` 与 `IN_MOVED_TO|IN_ISDIR` 时补 watch，add 后**立即 rescan** 补漏
2. **watch 挂 inode** → 监控「会被 safe-write 替换的文件」必失效；监控**目录** + `IN_MOVED_TO|IN_CLOSE_WRITE`
3. **读循环先判系统级事件** → `IN_Q_OVERFLOW`（全量重建）与 `IN_IGNORED`（清映射）在业务分支**之前**
4. **重活异步** → 事件循环只分派；消费速率跟不上 = 队列溢出只是时间问题
5. **改名降级安全** → MOVED_FROM/TO 处理成「删+建」永远正确；把「删+建」凑成改名是危险升级
6. **跨平台红线** → macOS/Windows 语义不对齐（无 name、无 cookie、无溢出事件）；抽象层按最小公倍数设计

---

## Pi5 复测清单（本章全部待实测）

| 项 | 节 | 验证方法 |
|----|----|---------|
| `inotify_dir` / `inotify_epoll` 编译运行、事件序列输出 | 19.4 | 一终端跑监控，另一终端操作目录比对 |
| 重复 `add_watch` 的 mask **替换** vs `IN_MASK_ADD` 叠加 | 19.2 | 两次 add 不同 mask，观察收到的事件集 |
| rename 被监控的**文件** → `IN_MOVE_SELF`，wd 存活 | 19.2/19.3 | `mv` 后继续观察事件 |
| 被监控文件被 safe-write 替换 → `IN_IGNORED`、监控失明 | 19.2 | 循环 rename 覆盖验证 |
| cookie 配对；跨挂载点移动时配不上 | 19.3 | 同目录 rename / `mv` 到另一挂载点 |
| 批量 read：一次 syscall 处理多条事件 | 19.4 | 批量创建 N 文件，统计 read 次数 |
| `IN_ONLYDIR` 对普通文件报 `ENOTDIR` | 19.2 | add_watch 一个普通文件 |
| `max_user_watches` 耗尽 → `ENOSPC` | 19.5 | 临时调小 sysctl 做实验 |
| `max_queued_events` 溢出 → `IN_Q_OVERFLOW`（wd=-1） | 19.5 | 调小队列 + 高频写 + 消费停顿 |
| `ex19_1`：watch 集合随子树增删动态更新 | 19.8 | 运行后在子树 mkdir/rmdir/mv，核对笔记预期输出 |
| `inotify_dtree` + `rand_dtree` 长跑无 mismatch | 19.4 | 并行跑两个程序压测一致性 |
| `dnotify.c` 行为（含一次性 vs `DN_MULTISHOT`） | 19.6 | 编译运行对照语义反转 |

---

## 参考

- Kerrisk · TLPI Ch19（19.1–19.8）
- man-pages 6.19：`inotify(7)` `inotify_init(2)` `inotify_add_watch(2)` `inotify_rm_watch(2)` `fcntl(2)`（F_NOTIFY/DN_*）`nftw(3)`
- Linux v6.6：`fs/notify/inotify/inotify_user.c`（read 拷贝语义、`IN_ONLYDIR`）、`include/uapi/linux/inotify.h`（mask 全表）
- 官方源码分发：`man7.org/tlpi/code/download/tlpi-260523-dist.tar.gz`（`inotify/` 共 4 个 .c + Makefile，本仓库全部逐字镜像）
- 习题编号核验：`github.com/segarciat/TLPI` `ch19-Monitoring-File-Events/exercises/`（仅 19-1）
- 交叉参考：`inotify_dtree.c` 的头注释是 Kerrisk 本人写的「inotify 健用法」说明，值得通读
