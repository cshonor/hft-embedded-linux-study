# TLPI 第 12 章 — System and Process Information

**优先级**：🔴（监控 / 调试 / 嵌入式与 HFT 观测**必读** `/proc`；Ch24 / Ch35 / Ch49 都要回头看这一章）
**前置**：[Ch03](../chapter-03-system-programming-concepts/README.md)（syscall 与 `errno` 范式——本章「脏 `errno`」全靠这个）· [Ch04](../chapter-04-file-io-universal/README.md)（读 `/proc` 用的就是普通 `open`/`read`）· [Ch11](../chapter-11-system-limits/README.md)（`sysconf` 与 `/proc` 是同一批数据的两个入口）
**后置**：[Ch13 文件 I/O 缓冲](../chapter-13-file-io-buffering/README.md)（`/proc` 也要过 stdio 缓冲）· [Ch17 访问控制](../chapter-17-access-control-lists/README.md)（`/proc/PID/status` 的 Uid 四元组）· [Ch24 进程创建](../chapter-24-process-creation/README.md) · [Ch35 进程优先级与调度](../chapter-35-process-priorities-scheduling/README.md)（`/proc/PID/stat` 的调度字段）· [Ch49 内存映射](../chapter-49-memory-mappings/README.md)（`/proc/PID/maps` 对照地址空间）

---

## 小节目录

- [12.1 The /proc File System `/proc` 文件系统](notes/12.1-proc-filesystem.md)
  - §12.1 `proc` 是一个「没有容量」的伪文件系统
  - §12.1.1 `/proc/PID` 下有什么
  - §12.1.2 整机信息（`/proc` 全局文件 + `sysinfo()`）
  - §12.1.3 怎么访问 `/proc` 文件
- [12.2 System Identification: uname() 系统标识](notes/12.2-uname.md)
- [12.3 Summary 本章总结](notes/12.3-summary.md)
- [12.4 Exercises 练习](notes/12.4-exercises.md)

> 四节的划分与 TLPI 原书一致（12.1–12.4）。本章**只有 12.1 有子编号小节**（12.1.1 / 12.1.2 / 12.1.3），所以 12.1 一篇里用 `###` 收三个子节，12.2 / 12.3 / 12.4 各一篇。
>
> ⚠️ **原书正文没有 `sysinfo()` / `get_nprocs()` 小节**（它们只在 12.1.2 的「整机信息」里被顺带提了一句）。本仓库为这两条线各写了一个**延伸 demo**，放在 12.4 的「延伸 A / 延伸 B」，并在文件头显式声明「原书没有这一节」——**不伪造节号**。
>
> ⚠️ **原书习题文字的公开原文未能核验**：man7 只分发源码、不放习题正文。但 **12-1 的官方解答程序是公开的**（`sysinfo/procfs_user_exe.c`，标注 "Solution to Exercise 12-1, page 231"，原书未印全文），本仓库逐字镜像了它，并据它反推出 12-1 的题面。12-2 / 12-3 只有任务要求描述，**不做逐字引用**。

---

## 章节目标

- **分清「两个入口」**：POSIX 可移植接口（`uname()` / `gethostname()` / `sysconf()` / `confstr()`）vs Linux 专有的「直接看内核」（`/proc`、`sysinfo()`）。**问错了入口，代价完全不同**——Ch11 那套在别的 Unix 上照样跑，`/proc` 在 BSD 上是另一个东西
- **`/proc` 不是一个「装文件的目录」**：它是一个**伪文件系统**（`f_type=0x9fa0`），`f_blocks=0`、`f_files=0`；文件的 `st_size=0` 但能读出 219 字节；内容在每次 `read` 时由内核**现算**
- **`/proc/PID/stat` 不要自己切**：`comm` 字段用一对括号包住，**第一个 `(` 要配最后一个 `)`**——comm 里含 `)` 时会整体错位
- **`sysinfo()` 有三个陷阱**：`procs` 是**线程数**（不是进程数）、`loads[]` 是 `1<<16` **定点数**、`uptime` **向上取整**
- **`/proc/sys` 写完要 `lseek` 回 0**：偏移不为 0 时内核**静默忽略**这次写入却照样返回成功
- **诚实**：本章 man-pages 6.19、glibc 2.39、Linux v6.6 的相关段落全部抓下来逐字核对，纠正了**书上一处**（`sysinfo().procs`）与我自己的**一处重大误判**（`gethostname()` 缓冲不足时其实是返回 `-1`/`ENAMETOOLONG`，不是静默截断）
- **溯源**：本章 **4 篇**笔记里的每一行实测输出都能在 CE 日志 `tlpi-ch12-final.txt` 里定位；每一处 glibc / 内核坐标都实读核准

### 一条主线：`/proc` 就是「内核把内部状态伪装成文件」

一个普通文件系统在磁盘上有 inode、有数据块、有容量；`/proc` 三样都没有：

| 普通文件系统（ext4 / tmpfs） | `/proc` |
|------------------------------|---------|
| `statfs()` 报 `f_blocks` / `f_files` > 0 | 实测 **`f_blocks = 0` / `f_files = 0`**，「容量」这个概念不成立 |
| `stat()` 的 `st_size` 是真实字节数 | 实测 `/proc/version` 的 **`st_size = 0`**，实际读出 **219** 字节 |
| 内容写在磁盘上，两次 `read` 一样 | 两次 `open+read` 的 `/proc/uptime` **差约 0.2 秒**（值在 `read` 时现算） |
| inode 由磁盘分配 | `st_ino` 是**内核现场编的**（实测 `4026532026`，落在 `0xF0000000` 段） |
| `open` 真的打开一个文件 | `/proc/self` 是**符号链接**（实测 `readlink` → `"2"`），由内核在路径解析时展开 |

一句话：**`/proc` 的每个文件都是一次「内核函数调用」被包装成了 `read()`**。这也解释了本章所有的反直觉行为——包括为什么**不要**对 `/proc` 文件做 `mmap`，以及为什么 `st_size` 不能拿来 `malloc`。

---

## 原书示例清单（man7 官方按章文件列表）

| Listing | 文件 | 页码 | 作用 |
|---------|------|------|------|
| **12-1** | `sysinfo/procfs_pidmax.c` | **p.228** | 读 / 写 `/proc/sys/kernel/pid_max`：`O_RDONLY` 读 → `lseek(fd,0,SEEK_SET)` → 写回原值 |
| **12-2** | `sysinfo/t_uname.c` | **p.230** | 调 `uname()` 打印六个字段，并**顺带演示 `gethostname()` 与 `nodename` 的关系** |
| **12-1 解答** | `sysinfo/procfs_user_exe.c` | **p.231** | **Solution to Exercise 12-1**（原书未印全文）：按 uid 遍历 `/proc/PID`，取 `stat` 的 comm 与 `exe` 的路径 |

> 出处：[man7 — List of source code files, by chapter](https://man7.org/tlpi/code/online/all_files_by_chapter.html) 的 **Chapter 12** 一节（共 **3 个**文件，都在 `sysinfo/` 下）。
> 本仓库在 `code/` 下**逐字镜像**了这 3 个文件（`sha256` 与原书一致），并提供 `tlpi_hdr.h` / `ugid_functions.h` 两个**最小替身**让它们能单文件编译——原书的这两个头依赖整个 `lib/` 目录（`lib/error_functions.c`、`lib/ugid_functions.c`），且以 `@` 结尾标记「已完成」。替身的语义**逐条对照原书真实实现**校准过（见 `code/README.md` 的「替身与原书的差异表」）。

三个容易踩的坑：

| 容易踩的坑 | 正解 |
|-----------|------|
| 以为 `st_size` 能拿来 `malloc` | **不能**。`/proc` 文件的 `st_size` 是 `0`（实测 `/proc/version` 读得出 219 字节）。必须开一个缓冲**读到 `read()` 返回 0** 为止 |
| 以为 `/proc/self/fd` 里那条指向自己的链接是「泄漏的 fd」 | 那是 `opendir("/proc/self/fd")` **自己**占的那个 fd（实测 `fd/3 -> /proc/2/fd`，就是本进程自己）。枚举时先 `closedir` 或不看它 |
| 写 `/proc/sys` 之后不检查返回值就以为成功了 | `write()` 返回 7（成功）但值没改——**偏移不为 0 时内核静默忽略**。写入前必须 `lseek(fd, 0, SEEK_SET)` |

---

## 易错清单

1. **`/proc/PID/stat` 的 `comm` 要用「第一个 `(` 配最后一个 `)`」** —— 实测 `prctl(PR_SET_NAME,"a)b")` 之后 `stat` 里是 `2 (a)b) R …`：正确切出 `(a)b)`（长 3），朴素切法得 `(a)`（长 1），**丢 2 个字符 → 后面约 50 个字段编号全部错位**。要进程名就用 `/proc/PID/comm`。
2. **`/proc/PID/status` 不能按行号取字段** —— 实测第 **2** 行是 `Umask`、第 **6** 行才是 `Pid`；顺序由内核 `fs/proc/array.c` 的 `seq_printf` 调用次序决定，跨内核版本会增删（`Umask` 就是较新内核才加的）。**必须按 key 名匹配**。（本沙箱 `/proc/self/status` 共 **61** 个 key。）
3. **`/proc` 文件的 `st_size` 是 0** —— 实测 `/proc/version`：`st_mode=0100444`、`st_size=0`、`st_blocks=0`、`st_nlink=1`，但读得出 **219** 字节。`fstat` 拿到的 size **不能**用来分配缓冲。
4. **`/proc` 的 `statfs` 里 `f_blocks = f_files = 0`** —— 实测 `f_type=0x9fa0`（`PROC_SUPER_MAGIC`）、`f_namelen=255`、`f_blocks=0`、`f_files=0`。任何「先看剩余空间够不够再写」的逻辑在 `/proc` 上都失效。
5. **`/proc/self` 是符号链接不是目录** —— 实测 `readlink("/proc/self")` → `"2"`（本进程 pid），`lstat` 看 `S_ISLNK=1`。`/proc/thread-self` → `"2/task/2"`。但 `open("/proc/self/status")` 照样能进——链接由内核解析，不经过磁盘。
6. **`cmdline` / `environ` 用 `\0` 分隔，不是空格也不是换行** —— 实测 `environ` 255 字节里 **7** 个 `\0`（= 7 条）。直接 `fgets` 只能拿到第一条；要全读得按 `\0` 切。而且条数**每次运行都不同**。
7. **`/proc/uptime` 的第二个数不是 idle 比例** —— 它是「**所有 CPU** 的空闲时间**总和**」。实测双核机器上约等于 2× 开机秒（`14702.22 24377.62`）。
8. **`/proc/loadavg` 的第 4/5 段不是负载** —— 第 4 段是 `running/total` **线程数**，第 5 段是「最近分配的 PID」。
9. **`sysinfo().procs` 是线程数，man page 写错了** —— man-pages 说 "Number of current processes"，但 `kernel/sys.c:2761` 赋的是 `info->procs = nr_threads;`。实测本沙箱 **242**，而 `/proc` 下只有 **2** 个进程目录。
10. **`sysinfo().loads[]` 是 `1<<16` 定点数** —— `loads[i] / 65536.0` 才是负载。实测 `58592/65536 = 0.8940` ↔ `/proc/loadavg` 的 `0.89`。忘了除就会把 0.89 读成 58592。
11. **`sysinfo().uptime` 是向上取整的** —— `kernel/sys.c:2757` 写的是 `tp.tv_sec + (tp.tv_nsec ? 1 : 0)`。实测 `sysinfo.uptime=14836` vs `/proc/uptime` 的 `14835.55`，**最多差 1 秒**。
12. **`mem_unit` 不能忽略** —— 正确读法是 `字段 × mem_unit`。x86-64 上 `mem_unit=1`（值已是字节），但 32 位大内存机上 `mem_unit=4096`（值是**页数**），直接当字节用会**少算 4096 倍**。实测 `totalram × mem_unit` 与 `/proc/meminfo` 的 `MemTotal × 1024` **严格相等**（同源：都来自 `si_meminfo()`）。
13. **`sysinfo()` 不做命名空间 / cgroup 隔离** —— 容器里报的是**宿主机**的线程数、内存量。被 PID 命名空间隔离的只是 `/proc` 的**进程目录**（这就是第 9 条里 242 vs 2 的根因）。`/proc/meminfo` 同样**不是 cgroup 口径**（第 12 条的「严格相等」就是证据）。要配额得读 cgroup。
14. **`uname()` 在 Linux 上就是裸的 `uname(2)`** —— glibc 通用实现（`posix/uname.c` 那份编译期常量版）**不是 Linux 用的**；Linux 走 `sysdeps/unix/syscalls.list:88` 的自动生成包装。实测 `uname(&u)` 与 `syscall(SYS_uname,&u)` 六字段全同。
15. **`syscall(SYS_uname, NULL)` 返回 `-1` / `EFAULT(14)`** —— 但这个 NULL 是 **`copy_to_user` 挡的**（`kernel/sys.c:1306-1321`），syscall 本身不检查 NULL（只有 `:1331` 的 `uname`/`olduname` 才显式 `if (!name) return -EFAULT;`）。`sysinfo(NULL)` 同理，**而且 `do_sysinfo()` 已经把活全干完了才在 `copy_to_user` 处失败——白干一趟**。
16. **`struct utsname` 的六个字段各 65 字节** —— `_UTSNAME_LENGTH = 65`，实测 `sizeof(struct utsname) = 390 = 6 × 65`，**没有填充**。`domainname` 是 Linux 特有、由 `#ifdef _GNU_SOURCE` 包住（拼接多文件时必须把 `#define _GNU_SOURCE` 提到**所有系统头之前**，否则这个字段会凭空消失）。
17. **`gethostname()` 缓冲不够时返回 `-1` / `ENAMETOOLONG`（36），而且缓冲里没有 NUL 终止** —— `sysdeps/posix/gethostname.c:38-42` 明写 `if (node_len > len) { __set_errno(ENAMETOOLONG); return -1; }`。实测 `len=1 → -1/36`、`len=2 → -1/36`、`len=3 → 0`。**返回 -1 时绝不能把缓冲当 C 字符串 `printf` 出去。**
18. **`gethostname()` 建在 `uname()` 之上** —— `sysdeps/posix/gethostname.c:26-46` 先 `__uname(&buf)` 再 `memcpy(name, buf.nodename, …)`。所以 `nodename ≡ gethostname()` 是**必然**一致，不是巧合。`misc/gethostname.c` 那份是 `ENOSYS` 桩，Linux 不用。
19. **写 `/proc/sys` 前必须 `lseek(fd, 0, SEEK_SET)`** —— 偏移不为 0 时 `fs/proc/proc_sysctl.c` 的 `proc_first_pos_non_zero_ignore()` 在 `sysctl_writes_strict=1`（默认）下**直接 return，`count` 照收**：实测「不清偏移直接 `write()` = 7 errno=0(ok)」而值没变。**`write` 返回成功 ≠ 值被改了**。
20. **对 `O_RDONLY` 打开的 knob 调 `write()` 是 `EBADF(9)`，不是 `EACCES`** —— 实测 `write()` → `errno=9`。而**用 `O_RDONLY` 打开一个可写 knob 会成功**（失败发生在 `write` 那一刻）。只读 knob（`osrelease`/`version`）用 `O_WRONLY` 打开才是 `EACCES(13)`。
21. **`SYS_sysctl` 系统调用已删除（Linux 5.5 起）** —— 实测本沙箱 glibc 头里**没有** `SYS_sysctl`。旧代码里的 `sysctl(3)` 一律改写成 `open/write("/proc/sys/...")`。更坑的是 glibc 里 `sysctl` 作为**兼容符号**还存在，**成功调用返回 0 但 buffer 未填**——比报错更难查。
22. **`/proc/sys/fs/file-max` 是系统级总量上限，不是「现在能开多少」** —— 实测 `9223372036854775807`（`LONG_MAX` 级）。单个进程还被 `RLIMIT_NOFILE` 卡着，查进程能开多少 fd 要 `getrlimit` / `sysconf(_SC_OPEN_MAX)`（见 [Ch11 §11.2](../chapter-11-system-limits/notes/11.2-runtime-limits.md)）。
23. **`get_nprocs()` 会留下脏 `errno`** —— 实测返回 `2` 但 `errno=2`（`ENOENT`）。逐行调用链：`getsysstats.c:216 read_sysfs_file("/sys/devices/system/cpu/online")` → `:148 __open_nocancel(...)` 失败 → `:151 if (fd != -1)` 不成立直接 `return 0`（**不清 errno**）→ `:217` 继续 → `:221 get_nprocs_fallback()` → `:198 get_nproc_stat()` 成功返回 2。**返回值对 ≠ errno 干净**；判错必须先 `errno = 0` 再调。
24. **`get_nprocs()` 报的可能是宿主机核数** —— glibc 那三条来源（`/sys/devices/system/cpu/online`、`/proc/stat`、`sched_getaffinity`）**没有一条读 cgroup 配额**。所以 `docker run --cpus=2` 的容器里完全可能返回宿主的 64。要真实并行度得自己读 cgroup 或看 `sched_getaffinity`。
25. **`statm` / `stat` 的单位是「页」，不是字节** —— 实测 `674 387 362 1 0 91 0`。乘 `sysconf(_SC_PAGESIZE)` 才是字节。
26. **环境决定一切**：本沙箱**无 `/bin/sh`**、**无 `/sys`**、`/etc/passwd` **只有 1 条**。所以 `procfs_user_exe.c` 的 uid 参数只能用**数字串**（走 `userIdFromName` 的 `strtol` 短路），`getpwnam("root")` 返回 `NULL`。

---

## 章节链路

```text
Ch2  进程 / 内核分工（谁提供这些数字）
  → Ch3  syscall 约定 + errno 范式（本章「脏 errno」「-1 是 EFAULT 还是 EACCES」全靠它）
  → Ch4  读 /proc = 普通 open + read（但 st_size 不可信、内容每次现算）
  → Ch10 时间（/proc/uptime 与 clock_gettime 的口径差别）
  → Ch11 系统限制与选项（sysconf 的可移植入口）
  → Ch12 /proc、uname()、sysinfo() —— 同一批数据的第二个入口
           /proc      内核把内部状态伪装成文件（Linux 专有）
           uname()    唯一 POSIX 可移植的「系统标识」入口
           sysinfo()  一次调用换一屏总量指标（Linux 专有，容器里报宿主机）
  → Ch13 stdio 缓冲（/proc 文件也要过 FILE* 缓冲）
  → Ch17 /proc/PID/status 的 Uid 四元组（real/effective/saved/fs）
  → Ch24 /proc/PID/stat 的 PPid（进程树）
  → Ch35 /proc/PID/stat 的调度字段（policy / nice / 优先级）
  → Ch49 /proc/PID/maps 对照 mmap 的地址空间
  → Ch55 文件锁（/proc/locks）
```

---

## 双线提示

| 路线 | |
|------|--|
| **HFT** | 行情网关的**自监控**几乎全靠 `/proc/self`：`fd/` 数连接、`statm` 看 RSS 增长、`status` 看 `Threads`/`VmRSS`/`voluntary_ctxt_switches`——但读它的**代价**要记住：每个 `/proc` 文件都是**一次 `seq_file` 生成**，高频轮询（每 tick 一次）会真的吃 CPU；**`st_size` 恒 0 所以不能用 `mmap`**，也不能预分配缓冲；`sysinfo().procs` 千万别当「本进程的线程数」上报（它是宿主级 `nr_threads`，实测 242 vs 2）——要看自己的线程数读 `/proc/self/status` 的 `Threads:`；负载阈值代码里 `loads[i] / 65536.0` 那个 `65536` 忘了写就是「负载 58592」这种假告警；`/proc/PID/stat` 自己解析迟早栽在 `comm` 里的 `)` 上（本仓实测 `a)b` 就丢 2 字符），生产环境要么用 `/proc/PID/comm`，要么用 libprocps；**容器化部署**时 `sysinfo()`、`/proc/meminfo`、`/proc/cpuinfo` 全是**宿主机口径**，按它们算的容量/并行度在 K8s 里必然错，要读 cgroup v2 的 `cpu.max` / `memory.max`。另外 `/proc/sys` 的写入**必须 `lseek` 回 0**，否则「write 成功但没生效」这种 bug 会在调参脚本里静默流传 |
| **嵌入式** | 交叉编译的板子上 `/proc` 常常被**裁剪**：`CONFIG_PROC_FS` 可以整个关掉（`sysinfo()` 会一起失效——`get_phys_pages()` 也是）；`/proc/cpuinfo` 在 ARM 上字段名与 x86 **完全不同**（是 `Hardware` / `Revision` / `Serial`，不是 `model name`）；`/proc/sys` 的可写项由 `CONFIG_*` 决定，同一份代码在两种内核配置下行为不同，**写之前先 `access()` 或直接读一把**；BusyBox 的精简系统里 `ps`/`free` 这些命令本身就是读 `/proc` 的壳，**没有 `/proc` 就全瞎**；`/proc/device-tree/`（或 `/sys/firmware/devicetree/`）是**只读**的 DT 快照——设备树调试第一站，但别把它当配置接口；只读根文件系统上 `/proc` 仍要**挂载**（`/proc` 是内核内存，不占 flash，挂上它零成本）；老 uClibc 的 `get_nprocs` 行为跟 glibc 差很多，别把 x86 上测出的「三路回退」当通用事实 |
| **两条线共同的坑** | 「`/proc` 文件看起来像文件，所以按文件用法写代码」——`st_size=0`、不可 `mmap`、内容每次现算、写入要 `lseek`。**把它当成「一次内核查询的文本化结果」而不是「一份磁盘内容」，本章所有的反直觉行为就都顺了** |

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 两个入口：**POSIX 可移植**（`uname`/`gethostname`/`sysconf`）vs **Linux 直接看内核**（`/proc`/`sysinfo`） |
| 2 | `/proc` 是**伪文件系统**：`f_type=0x9fa0`、`f_blocks=0`、`f_files=0`、「容量」不成立 |
| 3 | `/proc` 文件 **`st_size = 0`**，「大小」要**读到 EOF** 才知道；**不能 `mmap`** |
| 4 | `/proc` 内容**在 `read` 时现算**：两次读 `/proc/uptime` 差约 0.2 秒（`usleep` 那么多） |
| 5 | `/proc/self` 是**内核造的符号链接**（`readlink` → 自己的 pid），不是目录 |
| 6 | `/proc/PID/stat` 的 `comm` 用**第一个 `(` 配最后一个 `)`**；comm 含 `)` 时朴素切法全错位 |
| 7 | `/proc/PID/status` **按 key 名取**，别按行号（第 2 行是 `Umask`，第 6 行才是 `Pid`） |
| 8 | `cmdline` / `environ` 用 **`\0` 分隔**（不是空格/换行）；`fgets` 只能拿第一条 |
| 9 | `/proc/uptime` 第二个数是**所有 CPU 空闲时间之和**，不是 idle 比例 |
| 10 | `/proc/loadavg` 第 4 段 = `running/total` **线程数**，第 5 段 = 最近分配的 PID |
| 11 | 遍历 `/proc/PID/fd` 会看到**一条指向自己的链接**（`opendir` 自己占的 fd） |
| 12 | `statm` / `stat` 单位是**页**，不是字节 |
| 13 | `sysinfo().procs` = **`nr_threads`（线程数）**，man page 写错了；且是**宿主级** |
| 14 | `sysinfo().loads[]` 是 **`1<<16` 定点数**，读法 `loads[i]/65536.0` |
| 15 | `sysinfo().uptime` **向上取整**（`tv_sec + (tv_nsec ? 1 : 0)`），比 `/proc/uptime` 最多多 1 秒 |
| 16 | `sysinfo()` 的内存字段要 **`× mem_unit`**；x86-64 上 `mem_unit=1`，32 位大内存机上是 `4096` |
| 17 | `totalram × mem_unit` **严格等于** `/proc/meminfo` 的 `MemTotal × 1024`（同源 `si_meminfo()`） |
| 18 | `sysinfo()` **不做命名空间隔离**；容器里内存/线程数都是**宿主机**口径。要配额读 cgroup |
| 19 | `uname()` 在 Linux 上 = **裸 `uname(2)`**（`syscalls.list` 自动生成），不是 `posix/uname.c` 那份 |
| 20 | `struct utsname` = **6 × 65 = 390** 字节，无填充；`domainname` 需要 `_GNU_SOURCE` |
| 21 | `gethostname()` 建在 `uname()` 上 → `nodename ≡ gethostname()` 是**必然** |
| 22 | `gethostname(name, len)` 缓冲不够 → **`-1` / `ENAMETOOLONG(36)`，且缓冲无 NUL 终止** |
| 23 | 写 `/proc/sys` 前必须 **`lseek(fd, 0, SEEK_SET)`**；否则**静默忽略却返回成功** |
| 24 | 对 `O_RDONLY` 的 knob `write` → **`EBADF(9)`**（不是 `EACCES`）；只读 knob 用 `O_WRONLY` 开 → `EACCES(13)` |
| 25 | `SYS_sysctl` 自 **Linux 5.5** 起已删除；glibc 的 `sysctl` 只剩**兼容符号**（返回 0 但不填 buffer） |
| 26 | `get_nprocs()` / `sysconf(_SC_NPROCESSORS_*)` 会留**脏 `errno`**（`ENOENT`）；三路回退都不读 cgroup |
| 27 | `/proc/PID` 被 PID 命名空间隔离，但 **`sysinfo()` 不被隔离**——这就是「242 vs 2」的根因 |
| 28 | 原书本章只有 **3 个**示例文件，都在 `sysinfo/`：`procfs_pidmax.c`（**p.228**）/ `t_uname.c`（**p.230**）/ `procfs_user_exe.c`（**p.231**，12-1 的解答） |

---

## 参考

- Kerrisk, *The Linux Programming Interface*, **Chapter 12 — System and Process Information**
- [man7 官方源码清单（按章）](https://man7.org/tlpi/code/online/all_files_by_chapter.html) · [OUTLINE](../OUTLINE.md) · [Ch11 系统限制](../chapter-11-system-limits/README.md) · [Ch13 文件 I/O 缓冲](../chapter-13-file-io-buffering/README.md)
- man-pages **6.19**：`proc(5)`、`uname(2)`、`uname(3)`、`gethostname(2)`、`sysinfo(2)`、`sysconf(3)`、`procps(1)`、`cgroup(7)`、`namespaces(7)`
- POSIX.1-2017：`uname()`（`<sys/utsname.h>`）、《System Identification》
- 内核源码（**v6.6**）：`include/uapi/linux/utsname.h`（`__NEW_UTS_LEN`/`struct new_utsname`）、`include/linux/uts.h`（`UTS_SYSNAME`/`UTS_DOMAINNAME`）、`kernel/sys.c`（`SYSCALL_DEFINE1(newuname)` / `sethostname` / `gethostname` / `setdomainname` / `do_sysinfo` / `SYSCALL_DEFINE1(sysinfo)`）、`include/uapi/linux/sysinfo.h`（`SI_LOAD_SHIFT`/`struct sysinfo`）、`fs/proc/proc_sysctl.c`（`proc_first_pos_non_zero_ignore`）、`fs/proc/array.c`（`status` 的 `seq_printf` 次序）、`fs/proc/fd.c`（`proc_fd_link`/`proc_fdinfo`/`tid_fd_revalidate`）、`fs/proc/base.c`、`fs/proc/stat.c`、`fs/proc/meminfo.c`
- glibc **2.39**：`sysdeps/unix/syscalls.list`（`uname` 那行）、`sysdeps/unix/sysv/linux/bits/utsname.h`、`include/sys/utsname.h`、`posix/uname.c`、`posix/uname-values.h`、`posix/sys/utsname.h`、`sysdeps/posix/gethostname.c`、`misc/gethostname.c`、`sysdeps/unix/sysv/linux/getsysstats.c`（`__get_nprocs` / `__get_nprocs_conf` / `read_sysfs_file` / `get_nproc_stat` / `get_phys_pages` / `get_avphys_pages` / `sysinfo_mempages`）、`sysdeps/posix/sysconf.c`
- 原书真实 `lib/`（用于校准本仓替身语义）：`lib/tlpi_hdr.h`、`lib/ugid_functions.h`、`lib/ugid_functions.c:30-49`、`lib/error_functions.c:49-78`

---

## 代码示例

本章 `code/` 下有：

- **10 个自编 demo**（`c12_*.c` 7 个 + `ex12_*.c` 3 个）
- **3 个原书文件**逐字镜像（`procfs_pidmax.c` = Listing 12-1、`t_uname.c` = Listing 12-2、`procfs_user_exe.c` = 习题 12-1 解答），`sha256` 与原书一致
- **2 个框架替身**（`tlpi_hdr.h` 只含 `errExit`/`fatal`/`usageErr`/`cmdLineErr`；`ugid_functions.h` 是 `userIdFromName`/`userNameFromId` 的 `static inline` 版）

全部在 Compiler Explorer（gcc 13.3.0 / x86-64 / Ubuntu 24.04）上真实编译 + 运行过，共 **18 个作业**，全部 `build code = 0` / `didExecute = True` / **零诊断**（两个探针的 `-Wformat-truncation` 属故意为之；`procfs_user_exe_help` 的 `exit 1` 是 `usageErr` 的应然行为）。输出原样抄在对应笔记的实测块里，**共 531 行全部可回溯到冻结日志**。完整索引见 [`code/README.md`](code/README.md)。

| 文件 | 对应节 | 演示什么 | 需要什么 |
|------|--------|----------|---------|
| `c12_1_proc_nodes.c` | §12.1 | 钉住三个反直觉事实：`statfs` 的 `f_blocks=0`、`stat` 的 `st_size=0`、`/proc/self` 是符号链接；再用 `usleep` 证明内容**现算** | — |
| `c12_1_proc_pid.c` | §12.1.1 | `/proc/PID` 下有什么（`status`/`stat`/`statm`/`comm`/`maps`/`ns`/`cmdline`/`environ`/`fd`）+ 两个真实的解析陷阱（`comm` 含 `)`、`status` 第 2 行不是 `Pid`） | — |
| `c12_1_proc_global.c` | §12.1.2 | 整机信息，并把 `/proc` 与 `sysinfo()` **并排对账**（`totalram` 严格相等、`loads` 定点换算、`get_nprocs` 的脏 `errno`） | `/proc/meminfo` 可读 |
| `c12_1_proc_sys.c` | §12.1.3 | `/proc/sys` 写 knob 的**完整细节**：`O_RDONLY` 上 `write` 是 `EBADF`、不清偏移**静默忽略**、`lseek` 回 0 才生效、越界值 `EINVAL` | `/proc/sys` 可写（root） |
| `c12_2_uname.c` | 12.2 | `uname()` 八段：六个字段 / `'X'` 填充探测内核写了多少字节 / 与 `syscall(SYS_uname)` 对拍 / `NULL` → `EFAULT` / `gethostname` 逐 len 扫（`-1`/`ENAMETOOLONG` 且**无 NUL**）/ `domainname` 来由 | — |
| `c12_sysinfo.c` | 延伸 A | `struct sysinfo` 全字段 + `do_sysinfo()` 的四步流程（含 `mem_unit` 的两种情形）+ `procs` 是线程数的实证（242 vs 2）+ `get_phys_pages()` 就是它的换算 | — |
| `c12_nprocs.c` | 延伸 B | CPU 个数四条入口 / 底层三条来源逐条验证（`/sys` 不存在 → `ENOENT` → 数 `/proc/stat` → `sched_getaffinity`）+ 脏 `errno` 的**逐行调用链** | `/proc/stat` 可读 |
| `ex12_1_proc_walk.c` | 12.4 习题 12-1 | 按用户列出进程：自写 `uid_from_name()` + 遍历 `/proc/[0-9]*` + `read_stat_comm()` **双算法对比** + 竞态计数（`scanned`/`matched`/`skipped`/`vanished`） | 参数 `0` 或用户名 |
| `ex12_2_pstree.c` | 12.4 习题 12-2 | 画进程树：两遍扫描建 `PPid` 索引 → 递归打印，处理**孤儿 / 环 / 竞态消失** | — |
| `ex12_3_fd_snapshot.c` | 12.4 习题 12-3 | `/proc/self/fd` 快照：**readlink 前缀才是类型判据**（`socket:[..]` / `pipe:[..]` / `anon_inode:[..]`，`st_mode` 分不出来）+ 自指链接 + 两次快照差异 | — |
| `procfs_pidmax.c` | §12.1.3 | **原书 Listing 12-1（p.228）**：读 → `lseek(0)` → 写回 `pid_max` | `tlpi_hdr.h` + root |
| `t_uname.c` | 12.2 | **原书 Listing 12-2（p.230）**：`uname()` 打六个字段 | `tlpi_hdr.h` |
| `procfs_user_exe.c` | 12.4 习题 12-1 | **原书 12-1 官方解答（p.231）**：按 uid 遍历 `/proc/PID`，取 `stat` 的 comm 与 `exe` 的路径 | `tlpi_hdr.h` + `ugid_functions.h` + root |

一次编完全部自编 demo（在 `code/` 目录下）：

```bash
for f in c12_*.c ex12_*.c; do
    gcc -O0 -Wall -Wextra -o "${f%.c}" "$f" || echo "FAIL $f"
done
```

3 个原书程序需要头替身（原书 `main(argc, argv)` 不用这两个参数，原书 Makefile 只开 `-Wall`，我们额外加 `-Wextra` 时要带 `-Wno-unused-parameter`）：

```bash
gcc -O0 -Wall -Wextra -Wno-unused-parameter -o t_uname tlpi_hdr.h t_uname.c
gcc -O0 -Wall -Wextra -Wno-unused-parameter -o procfs_pidmax tlpi_hdr.h procfs_pidmax.c
gcc -O0 -Wall -Wextra -Wno-unused-parameter -o procfs_user_exe tlpi_hdr.h ugid_functions.h procfs_user_exe.c
```

运行示例：

```bash
./c12_1_proc_nodes        # statfs / st_size / /proc/self 三个反直觉事实
./c12_1_proc_pid          # /proc/PID 全家桶 + 两个解析陷阱
./c12_1_proc_global       # 整机信息 + 与 sysinfo() 对账
./c12_1_proc_sys          # /proc/sys 写入的完整细节（建议 root）
./c12_2_uname             # uname 八段
./c12_sysinfo             # sysinfo 十段
./c12_nprocs              # CPU 个数四条入口
./ex12_1_proc_walk 0      # 列出 uid=0 的进程
./ex12_2_pstree           # 画进程树
./ex12_3_fd_snapshot      # 自己的 fd 快照
./t_uname                 # 原书 Listing 12-2
./procfs_pidmax           # 原书 Listing 12-1（root）
./procfs_user_exe 0       # 原书 12-1 解答（root；沙箱 /etc/passwd 只有 1 条，须用数字）
./procfs_user_exe --help  # 看 usageErr 的文案
```

**几条实测结论**（都是本仓库跑出来的，不是书上抄的）：

- **`/proc` 的「不是文件」三连**：`statfs` → `f_type=0x9fa0` / `f_blocks=0` / `f_files=0`；`stat("/proc/version")` → `st_mode=0100444` / `st_size=0` / `st_blocks=0`，实际读出 **219** 字节；`open+read("/proc/uptime")` 两次（间隔 `usleep(200ms)`）→ `306.48 252.19` / `306.68 252.58`
- **`/proc/self` 是符号链接**：`readlink` → `"2"`（`getpid()==2`）；`/proc/thread-self` → `"2/task/2"`；`lstat` 的 `S_ISLNK=1`
- **`/proc` 顶层构成本次是 2 + 62**：数字目录（进程）**2** 个、其它条目（内核接口）**62** 个
- **两个解析陷阱**：`status` 第 2 行是 `Umask`（`Pid` 在第 6 行）、共 **61** 个 key；`prctl(PR_SET_NAME,"a)b")` 后 `stat` 里 `2 (a)b) R …`，正确切法 `(a)b)`（长 3）vs 朴素切法 `(a)`（长 1）→ **丢 2 个字符**
- **`cmdline` / `environ` 是 NUL 分隔**：`cmdline` 11 字节 / **1** 个 NUL；`environ` 255 字节 / **7** 个 NUL（7 条）
- **`fd/3 -> /proc/2/fd`**：枚举 `/proc/self/fd` 时 `opendir` 自己占的那个 fd 也在列表里，目标就是本进程的 fd 目录
- **`uname()` 在 Linux 上就是裸 syscall**：`uname(&u)` 与 `syscall(SYS_uname,&u)` **六字段全同**；`sizeof(struct utsname) = 390 = 6 × 65`（无填充）；`syscall(SYS_uname,NULL)` → `-1` / `EFAULT(14)`
- **`gethostname` 缓冲不足返回 `-1` 且不终止**：`len=1 → -1/36`、`len=2 → -1/36`、`len=3 → 0`；`sysconf(_SC_HOST_NAME_MAX) = 64`
- **`sysinfo()` 的三个陷阱实测**：`procs = 242` 而 `/proc` 下只有 **2** 个进程目录（宿主级 `nr_threads`）；`loads[0] = 58592` → `/65536 = 0.8940` ↔ `/proc/loadavg` `0.89`；`uptime = 14836` vs `/proc/uptime` `14835.55`（**向上取整**）
- **`mem_unit` 与对账**：`totalram × mem_unit = 16465022976` = `/proc/meminfo` 的 `MemTotal 16079124 kB × 1024`，**严格相等**；`get_phys_pages() = 4019781` 恰好 `= totalram × mem_unit / PAGE_SIZE`
- **`uname(NULL)` / `sysinfo(NULL)`**：都是 `-1` / `EFAULT(14)`，都在 `copy_to_user` 处被挡（且 `do_sysinfo` 已把活干完）
- **写 `/proc/sys` 的三态**：对 `O_RDONLY` 的 fd `write` → `EBADF(9)`；不清偏移直接 `write` → **返回 7 但值没改**；`lseek` 回 0 再 `write` 越界值 `"0"` → `EINVAL(22)`，写原值 `"4194304"` → 成功
- **只读 knob 的权限**：`open(osrelease, O_WRONLY)` / `open(version, O_WRONLY)` → `EACCES(13)`；但 `open(pid_max, O_RDONLY)` **成功**（失败推迟到 `write`）
- **`get_nprocs()` 四入口全是 2**：`get_nprocs()` / `get_nprocs_conf()` / `sysconf(_SC_NPROCESSORS_ONLN)` / `sysconf(_SC_NPROCESSORS_CONF)` 都是 `2`，**且 `errno` 全是 `ENOENT(2)`**（脏值）；自己数 `/proc/stat` 的 `cpuN` = 2；`CPU_COUNT(sched_getaffinity(0))` = 2
- **`SYS_sysctl` 不存在**：本沙箱 glibc 头里没有这个宏（Linux 5.5 起已删除）
- **`/proc/sys` 关键值**：`pid_max = 4194304`、`threads-max = 54382`、`ngroups_max = 65536`、`randomize_va_space = 2`、`nr_open = 1048576`、`somaxconn = 4096`、`file-max = 9223372036854775807`（`LONG_MAX` 级）

> ⚠️ **会漂移的数字**（**重跑必变，不要在当前笔记里硬编码**）：`uptime`、`loads[]`、`freeram`、`freeswap`、`procs`、`/proc` 顶层计数、`/proc/PID/stat` 的 `vsize` / `rss` / 时间片字段、`maps` 的行数与 inode、`ns/` 的 inode 号、`environ` 的条数、`get_avphys_pages()`、`fd` 快照的个数与 inode、`statfs` 各文件系统的 `f_blocks`。
> **相对稳定**：`totalram`、`totalswap`、`mem_unit`、`totalhigh`、`freehigh`、`sizeof(struct sysinfo) = 112`、`sizeof(struct utsname) = 390`、`_SC_HOST_NAME_MAX = 64`、`f_type`、`f_namelen`。
> ⚠️ CE 的容器会被调度到**不同宿主机**，`totalram` 都可能变（本仓库曾抓到 `16465022976`（16.5 GB）与 `8154017792`（8.15 GB）两次）。**本章所有实测数字均来自同一次冻结运行 `tlpi-ch12-final.txt`。**

---

## 与前后章

| 章 | 关联 |
|----|------|
| [Ch11](../chapter-11-system-limits/README.md) | `sysconf` 是同一批数据的**可移植入口**；本章 `/proc`/`sysinfo` 是**Linux 直读入口**。移植优先 Ch11，刨根问底用 Ch12 |
| [Ch04](../chapter-04-file-io-universal/README.md) / [Ch13](../chapter-13-file-io-buffering/README.md) | 读 `/proc` 就是普通文件 I/O + 缓冲细节（但 `st_size` 不可信） |
| [Ch03](../chapter-03-system-programming-concepts/README.md) | 「脏 `errno`」「`-1` 是 `EFAULT` 还是 `EACCES`」全章判据 |
| [Ch17](../chapter-17-access-control-lists/README.md) | `/proc/PID/status` 的 `Uid:`/`Gid:` 是 **real/effective/saved/fs 四元组** |
| [Ch24](../chapter-24-process-creation/README.md) / [Ch35](../chapter-35-process-priorities-scheduling/README.md) | 进程树（`PPid`）、调度字段（`policy`/`nice`/`prio`）都在 `/proc/PID/stat` |
| [Ch49](../chapter-49-memory-mappings/README.md) | `/proc/PID/maps` 是 mmap 结果的**文本视图** |
| [Ch55](../chapter-55-file-locking/README.md) | `/proc/locks` 是全局锁表 |
