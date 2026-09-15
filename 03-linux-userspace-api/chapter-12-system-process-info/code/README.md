# Ch12 `code/` 目录说明

TLPI 第 12 章（System and Process Information）的可编译代码。分三类：

| 类别 | 数量 | 命名 |
|------|------|------|
| 自编 demo | **10** | `c12_<节>_<名字>.c`（7 个）+ `ex12_<n>_<名字>.c`（3 个） |
| 原书镜像 | **3** | `procfs_pidmax.c`(Listing 12-1)、`t_uname.c`(Listing 12-2)、`procfs_user_exe.c`(习题 12-1 解答)，**逐字保真**（`sha256` 与原书一致） |
| 框架替身 | **2** | `tlpi_hdr.h`（含 `errExit` / `fatal` / `usageErr` / `cmdLineErr`）+ `ugid_functions.h`（`userIdFromName` / `userNameFromId`） |

**全部 18 个作业**都在 Compiler Explorer（gcc 13.3.0 / x86-64 / Ubuntu 24.04）上跑过：`build code = 0`、`didExecute = True`、**`diagnostics = 0`**。唯二的「非零」都在白名单里：两个探针的 `-Wformat-truncation`（故意用 128 字节缓冲喂 `snprintf("/proc/self/fd/%s")`）与 `procfs_user_exe --help` 的 `exit 1`（`usageErr` 的应然行为）。

> 为什么走 Compiler Explorer：本机环境里 `wsl.exe` 被安全策略禁用，且没有任何 C 编译器（`gcc`/`clang`/`tcc`/`cl`/`cc`/`zig` 全无）。CE 提供真实的 gcc 13.3 编译诊断与真实运行输出；笔记里凡引用输出都标注「CE 实测」，**不当成本机实测**。
>
> ⚠️ CE 是**一次性容器**（每次执行都是全新的，所以本目录里改写 `/proc/sys/kernel/pid_max` 不会跨次残留——另有 `restore_pidmax.c` 双保险），而且是个**功能残缺的容器**：**没有 `/bin/sh`**、**没有 `/sys`（整棵 `/sys/devices/system/cpu` 不存在）**、`/etc/passwd` **只有 1 条**（`ce:x:10240:10240:…:/app:/bin/bash`，所以 `getpwnam("root")` 返回 `NULL`）、**没有 `SYS_sysctl`**、`/proc` 下**只有 2 个进程目录**。所以本章所有交叉验证都靠**程序内部打两遍**，不靠外部命令。

---

## 文件表

### 自编 demo（10）

| 文件 | 对应节 | 演示什么 | 需要什么 |
|------|--------|----------|---------|
| `c12_1_proc_nodes.c` | §12.1 | 钉住三个反直觉事实：**①** `statfs` 对比四个挂载点 —— `/proc` 是 `f_type=0x9fa0` 且 `f_blocks=0`/`f_files=0`，`/`、`/dev` 是 tmpfs 有正常容量；**②** `stat("/proc/version")` 的 `st_size=0` 但读得出 219 字节；**③** `/proc/self` 不是目录而是内核造的符号链接（`readlink` → `"2"`）；**④** `usleep` 之后重读 `/proc/uptime`，秒数差约 0.2 —— 证明内容**在 `read` 时现算**；**⑤** 数 `/proc` 顶层的数字目录 vs 其它条目 | — |
| `c12_1_proc_pid.c` | §12.1.1 | `/proc/PID` 下有什么：`status`（前 8 行 + 61 个 key 计数）/ `stat`（单行长什么样）/ `statm` / `comm` / `maps` / `ns/`（10 个命名空间链接）/ `cmdline`+`environ`（**NUL 分隔**，NUL 显示为 `|`）/ `cwd`+`exe`+`root`+`fd/`。外加**两个真实的解析陷阱**：`comm` 里含 `)` 时朴素切法丢字符、`status` 第 2 行不是 `Pid` 而是 `Umask` | — |
| `c12_1_proc_global.c` | §12.1.2 | 整机信息**并排对账**：`/proc/version`/`uptime`/`loadavg` 原样一行；`sysinfo()` 打 `totalram`/`freeram`/`loads`/`procs`/`mem_unit`，并**与 `/proc/meminfo` 逐项比**（`totalram × mem_unit` 严格相等）；`loads[i]/65536.0` ↔ `/proc/loadavg`；CPU 个数的四条入口；`/proc/sys/kernel/{hostname,osrelease}` ↔ `gethostname()`/`uname().release`；`/proc/sys/fs/file-max` 的误导性 | `/proc/meminfo` 可读 |
| `c12_1_proc_sys.c` | §12.1.3 | `/proc/sys` 写入的**完整细节**：**①** 扫一层树的 11 个 knob；**②** 用三种 flag 分别 `open` 可写与只读 knob（可写 knob 用 `O_RDONLY` 打开**会成功**，只读 knob 用 `O_WRONLY` 才是 `EACCES(13)`）；**③** 写 `pid_max` 四步：对 `O_RDONLY` 的 fd `write`→`EBADF(9)` / 不清偏移 `write`→**返回 7 但值没改**（静默忽略）/ `lseek` 回 0 再写越界值→`EINVAL(22)` / 写原值→成功；**④** 证明 `SYS_sysctl` 在本机 glibc 头里不存在 | `/proc/sys` 可写（root） |
| `c12_2_uname.c` | 12.2 | `uname()` 八段：**①** 六个字段 + `sizeof`；**②** 每个字段的**声明长度**（65）与**实际长度**；**③** 先用 `'X'` 灌满缓冲再 `syscall(SYS_uname, &raw)`，数「内核没写到的字节」→ 证明整份 390 字节载荷都被填满，再演示 `NULL` → `EFAULT(14)`；**④** `uname(&u)` 与 `syscall(SYS_uname, &u)` **六字段对拍**；**⑤** `gethostname(name, len)` 的 `len` 从 1 扫到 5，并 hexdump —— `len=1/2` 是 `-1`/`ENAMETOOLONG(36)` 且**缓冲无 NUL 终止**；**⑥** `sysconf(_SC_HOST_NAME_MAX)`；**⑦** `domainname = "(none)"` 的来由；**⑧** 列出 `uname()` 拿不到的三样东西 | — |
| `c12_sysinfo.c` | **延伸 A**（原书**没有**这一节） | `struct sysinfo` 十段：**①** 全字段（含 `sizeof = 112`）；**②** `do_sysinfo()` 的四步流程与 `mem_unit` 的两种情形；**③** `totalram × mem_unit` 与 `/proc/meminfo MemTotal` 对账；**④** `loads[]` 的 `1<<16` 定点换算；**⑤** `uptime` 向上取整的实证（对比 `/proc/uptime` 的浮点秒）；**⑥** `procs` 是**线程数**且是**宿主级**（242 vs `/proc` 的 2）；**⑦** `totalhigh`/`freehigh` 恒 0；**⑧** `get_phys_pages()` 就是它的换算（含 glibc 源码注释原话）；**⑨** `sysinfo(NULL)` → `EFAULT`；**⑩** 会漂移的字段清单 + 命名空间/cgroup 说明 | — |
| `c12_nprocs.c` | **延伸 B**（原书**没有**这一节） | CPU 个数七段：**①** 四个入口（`get_nprocs` / `get_nprocs_conf` / 两条 `sysconf`）**故意分成「调用」与「取 errno」两条语句**；**②** 验证第一条来源 `/sys/devices/system/cpu/{online,possible,present}` 在本容器全 `ENOENT`；**③** 自己数 `/proc/stat` 的 `cpuN` 行（并示范 `isdigit(l[3])` 为什么必要）；**④** `sched_getaffinity(0)` 的 `CPU_COUNT`；**⑤** 把脏 `errno` 的**逐行调用链**写全（`getsysstats.c:216 → :148 → :151 → :217 → :221 → :198`）；**⑥** 容器里它可能报宿主核数（三条来源都不读 cgroup）；**⑦** 六个数合账 | `/proc/stat` 可读 |
| `ex12_1_proc_walk.c` | 12.4 习题 12-1 | 按用户列出进程：自写 `uid_from_name()`（照 `lib/ugid_functions.c:30-49` 的三条语义）+ 遍历 `/proc/[0-9]*` + **同一份 `stat` 用两种算法切 `comm` 并对比** + 竞态计数（`scanned` / `matched` / `uid 不匹配跳过` / `扫描中消失`） | 参数（uid 数字串或用户名） |
| `ex12_2_pstree.c` | 12.4 习题 12-2 | 画进程树：**两遍扫描**（先建 `pid → PPid` 索引，再递归打印）——因为 `/proc` 目录的枚举顺序**不是**父先子后；处理三类异常：**孤儿**（父不在快照里）、**环**（`PPid` 链自指）、**竞态消失**（读到一半进程没了） | — |
| `ex12_3_fd_snapshot.c` | 12.4 习题 12-3 | `/proc/self/fd` 快照：**①** 枚举 + `readlink` 打每个 fd 的目标；**②** `kind_of()` **先看 readlink 前缀**（`socket:` / `pipe:` / `anon_inode:` / `memfd:`），说明为什么 `fstat().st_mode` 分不出「磁盘普通文件」与「内核匿名文件」（都是 `S_IFREG`）；**③** 那条**指向自己**的链接（`fd/3 -> /proc/2/fd`）；**④** 手工开几个 fd 再快照，看两次的差值；**⑤** 用 `fdinfo` 补上 `flags` / `pos`；**⑥** 说明「快照只是某一瞬间的像」 | — |

### 原书镜像（3，逐字保真）

| 文件 | 原书位置 | 说明 | 需要什么 |
|------|---------|------|---------|
| `procfs_pidmax.c` | **Listing 12-1, p.228** | 读 → 写回 `/proc/sys/kernel/pid_max`。核心是那三行：`open(O_RDONLY)` → `read` 到字符串 → **`lseek(fd, 0, SEEK_SET)`** → `write`。**那个 `lseek` 就是全章的隐藏考点**：不写它，内核会**静默忽略**这次写入却照样返回成功 | `tlpi_hdr.h` + root |
| `t_uname.c` | **Listing 12-2, p.230** | `uname()` 打六个字段，然后**顺带调 `gethostname()`** 打印 `nodename`——原书用这一手暗示「两者是同一个来源」 | `tlpi_hdr.h` |
| `procfs_user_exe.c` | **Solution to Exercise 12-1, p.231**（原书未印全文） | 按 uid 遍历 `/proc/PID`：`userIdFromName()` 把参数转 uid → `opendir("/proc")` 筛数字目录 → 读 `stat` 取 `comm` → 读 `exe` 符号链接取路径。末尾是 `usageErr` 的标准用法 | `tlpi_hdr.h` + `ugid_functions.h` + root |

> 页码出处：man7 单文件页原文（`This is sysinfo/procfs_pidmax.c (Listing 12-1, page 228), an example from the book, The Linux Programming Interface.` 等）。
> 镜像来源：`https://man7.org/tlpi/code/online/dist/sysinfo/<file>`。
> 文件清单出处：[man7 — List of source code files, by chapter](https://man7.org/tlpi/code/online/all_files_by_chapter.html) 的 **Chapter 12** 一节——共 **3 个**文件，都在 `sysinfo/` 下。
> 逐字校验：三个文件的 `sha256` 与原书下载件一致（`81ccb2973674670a…` / `f9558df03a5ed745…` / `c59e5974bd9de80e…`）。

⚠️ **本章原书只有 3 个示例文件**，但**实测数据是全模块最多的章之一**——真正的知识量在「拿 `/proc` 去问一个真实内核对不对」。

⚠️ **习题原文未逐字引用（诚实标注）**：`ex12_1_*` / `ex12_2_*` / `ex12_3_*` 的头注释里只写【任务】，不引原书题干——因为 man7 只分发源码、不放习题正文。**例外是 12-1**：它的官方解答程序 `procfs_user_exe.c` 是公开的，所以 12.1 的题面由该程序**反推**得出，并在笔记里标注了推理依据。

### 框架替身（2）

| 文件 | 说明 |
|------|------|
| `tlpi_hdr.h` | 原书 `lib/tlpi_hdr.h` 的**最小可用子集**，按原书语义实现 4 个助手：`errExit`（`fmt: strerror(errno)` + `exit(1)`）/ `fatal`（带 `"ERROR: "` 前缀，**注意原书带冒号**）/ `usageErr`（打 `Usage: ` 前缀 + `exit(1)`）/ `cmdLineErr`（前缀 `"Command-line usage error: "`，**与原文逐字一致**）。原书结构是「`tlpi_hdr.h` 声明 + `error_functions.c` 实现」，为单文件可控，这里把实现做成 `static inline` 放在头里 |
| `ugid_functions.h` | 原书 `lib/ugid_functions.h` + `ugid_functions.c` 的 `static inline` 版，只含本章用到的两个函数。`userIdFromName()` 的三条语义**逐条对齐** `lib/ugid_functions.c:30-49`：① `NULL` / 空串 → `(uid_t)-1`；② `strtol` 后 `*endptr == '\0'` → **直接返回数字，不查 passwd**；③ 否则 `getpwnam()`，查不到返回 `(uid_t)-1` |

> ⚠️ **本章替身与 `lib/error_functions.c` 的差异（3 条，必须知道）**：
> 1. **报文格式退化**：原书 `outputError()`（`lib/error_functions.c:49-78`）打的是 `"ERROR%s %s\n"`，其中 `errText` 为 `" [%s %s]"`（= `ename[err]` + `strerror(err)`），也就是**同时给人话和错误名**；本替身只打 `strerror` 文本。
> 2. **只提供 4 个入口**：原书有 7 个（`errMsg` / `errExit` / `err_exit` / `errExitEN` / `fatal` / `usageErr` / `cmdLineErr`），本章只用到 `errExit` / `fatal` / `usageErr` / `cmdLineErr`。
> 3. **`usageErr` / `cmdLineErr` 的文案逐字保留**，因为本章的 `procfs_user_exe.c` **会真的触发它们**（`--help` 那条作业就是去验证 `usageErr` 的文案与 exit code）。
>
> 另外原书 `error_functions.c` 会 `cache` 环境变量 `EF_DUMPCORE` 来决定 `abort()` 还是 `exit`/`_exit`；替身不做这件事（本仓库不测 core dump）。
>
> ⚠️ 这份 `tlpi_hdr.h` 与 Ch04 / Ch05 / Ch10 / Ch11 的同名替身**不能互换**（每章只保留自己用到的最小集）。别跨章复制粘贴——这是本仓库故意保持的「每章最小化」策略。

---

## 编译

一次编完全部自编 demo（在 `code/` 目录下）：

```bash
for f in c12_*.c ex12_*.c; do
    gcc -O0 -Wall -Wextra -o "${f%.c}" "$f" || echo "FAIL $f"
done
```

10 个自编 demo 里要用 `struct utsname.domainname` 的那个（`c12_2_uname.c`）在源码开头自带 `#define _GNU_SOURCE`，**不需要额外旗标**；`ex12_1_proc_walk.c` 需要 `<sys/prctl.h>`（已包含）。

3 个原书程序都需要头替身（**同目录即可**）：

```bash
gcc -O0 -Wall -Wextra -Wno-unused-parameter -o t_uname tlpi_hdr.h t_uname.c
gcc -O0 -Wall -Wextra -Wno-unused-parameter -o procfs_pidmax tlpi_hdr.h procfs_pidmax.c
gcc -O0 -Wall -Wextra -Wno-unused-parameter -o procfs_user_exe tlpi_hdr.h ugid_functions.h procfs_user_exe.c
```

> `-Wno-unused-parameter` 是因为 `t_uname.c` 的 `main(int argc, char *argv[])` **根本不用这两个参数**。原书 Makefile 只开 `-Wall`，我们额外加的 `-Wextra` 会把它报成 `warning: unused parameter`。**原书代码一字不改**，改旗标。
>
> ⚠️ 拼接多个文件时有一个隐蔽陷阱：`#define _GNU_SOURCE` 必须落在**任何系统头之前**。原书 `procfs_user_exe.c` 只 `#include "tlpi_hdr.h"`，而下面这条命令把 `tlpi_hdr.h` 和 `ugid_functions.h` 都传了进去——若手工拼接，稍不注意 `#define _GNU_SOURCE` 就会落到 `<stdio.h>` 之后，`struct utsname.domainname` 会凭空消失。本仓库的编译脚本 `gen_ce_single.py` 用正则把所有 feature-test 宏**提升到拼接文件顶部**来根治这件事。

## 运行示例

```bash
./c12_1_proc_nodes        # statfs / st_size / /proc/self 三个反直觉事实 + 内容现算
./c12_1_proc_pid          # /proc/PID 全家桶 + 两个解析陷阱
./c12_1_proc_global       # 整机信息 + 与 sysinfo() 对账
./c12_1_proc_sys          # /proc/sys 写入的完整细节（建议 root）
./c12_2_uname             # uname 八段
./c12_sysinfo             # sysinfo 十段（延伸 A）
./c12_nprocs              # CPU 个数四条入口（延伸 B）
./ex12_1_proc_walk 0      # 列出 uid=0 的进程
./ex12_2_pstree           # 画进程树
./ex12_3_fd_snapshot      # 自己的 fd 快照

# 原书程序
./t_uname                       # Listing 12-2
./procfs_pidmax                 # Listing 12-1（root；会读写 pid_max）
./procfs_user_exe 0             # 习题 12-1 解答（root；沙箱 /etc/passwd 只有 1 条，须用数字）
./procfs_user_exe --help        # 看 usageErr 的文案（exit 1 是应然）
```

## 沙箱环境注意（这些「失败」是环境限制，不是代码 bug）

| 现象 | 原因 |
|------|------|
| 没有 `/bin/sh` | 实测 `system("echo hello-from-sh") = 32512`（`WIFEXITED=1`、`status=127`、`errno=2`）。所以本目录**没有一处**用 `system()` / `popen()`——`c12_sysinfo.c` 要数 `/proc` 下的进程数是直接用 `opendir`/`readdir` 自己数的 |
| **整棵 `/sys` 不存在** | 实测 `opendir("/sys/devices/system/cpu")` 失败，`/sys/devices/system/cpu/{online,possible,present}` 全 `ENOENT`；`/sys/fs/cgroup/cpu.max` 也不存在。这正好让 `c12_nprocs.c` 演示了 glibc 的**三路回退**（`/sys` 失败 → 数 `/proc/stat` → `sched_getaffinity`） |
| `/etc/passwd` **只有 1 条**（`ce:x:10240:10240:Not a real account:/app:/bin/bash`） | 所以 `getpwnam("root")` / `getpwnam("0")` 都返回 `NULL`。原书 `procfs_user_exe.c` 必须传**数字串** `"0"`，走 `userIdFromName()` 的 `strtol` 短路分支 |
| `geteuid()=0`、`getuid()=0` | 容器是 root，所以 `/proc/sys/kernel/pid_max` **可写**（这是 `c12_1_proc_sys.c` 与 Listing 12-1 能跑起来的前提）。**真机上非 root 会 `EACCES`** |
| `/proc` 下**只有 2 个进程目录** | 实测 `/proc` 顶层数字目录 **2** 个、其它条目 **62** 个；`/proc/self` → `"2"`。所以进程树的样本极少（`ex12_2_pstree` 只有 1~2 行）——**这不是程序 bug，是容器太干净** |
| `sysinfo().procs = 242` 而 `/proc` 只有 2 个进程目录 | **`nr_threads` 是宿主级计数器，`/proc` 只列本 PID 命名空间**。`sysinfo()` / `/proc/meminfo` 都**不做命名空间/cgroup 隔离**——所以要配额得读 cgroup。这是本章最值钱的一条实测 |
| `totalram` 在两次不同运行里分别是 `16465022976`（16.5 GB）与 `8154017792`（8.15 GB） | **CE 会把容器调度到不同宿主机**。本仓库为此把本章**所有**引用数字冻结在同一次运行（`tlpi-ch12-final.txt`）里，并把「会漂移的字段」单独列成一张表。**别把某一台机的数字当常量** |
| `get_nprocs()` 返回 `2` 但 `errno == 2`（`ENOENT`） | **返回值对，`errno` 是脏的**：`getsysstats.c:216` 那条 `read_sysfs_file("/sys/devices/system/cpu/online")` 失败后，`:151` 的 `if (fd != -1)` 不成立直接 `return 0`，**不清 `errno`**。教训：**判 `errno` 前必须先 `errno = 0`**，且「调用」与「取 `errno`」必须分成两条语句（写成 `printf("%d %d", f(), errno)` 会因求值顺序未定义而读到旧值） |
| 对 `O_RDONLY` 的 `/proc/sys/...` fd 调 `write` → `EBADF(9)` | 不是 `EACCES`。fd 本身合法，只是**不是为写打开的**。而**用 `O_RDONLY` 打开一个可写 knob 会成功**——失败推迟到 `write` 那一刻 |
| 不清偏移直接 `write("/proc/sys/...")` 返回 **7**（成功）但值没变 | `fs/proc/proc_sysctl.c` 的 `proc_first_pos_non_zero_ignore()` 在 `sysctl_writes_strict=1`（默认）下**直接 return，`count` 照收**。**`write` 返回成功 ≠ 值被改了**——这就是原书 Listing 12-1 里那句 `lseek(fd, 0, SEEK_SET)` 的存在理由 |
| `SYS_sysctl` 未定义 | 这个系统调用自 **Linux 5.5** 起已删除，glibc 也不再导出包装。旧代码的 `sysctl(3)` 一律改写成 `open/write("/proc/sys/...")` |
| `pid_max` 被探针改写 | CE 每次执行都是**全新容器**（实测 `restore_pidmax.c` 读到的 `before = [4194304]`），所以**无跨次残留**。仍在探针末尾与独立程序里各做了「写回原值」的双保险 |
| `/proc/version` 报 `st_size = 0` | **正常**。`/proc` 文件的大小要读到 `EOF` 才知道。任何「先 `fstat` 看 size 再 `malloc`」的写法在 `/proc` 上都是错的 |
| `environ` 的条数每次都不同（实测 **7** 条） | 容器启动时注入的环境变量不同。**别硬编码条数** |
| `uname().release = 7.0.0-1012-aws`、`nodename = ce`、`domainname = (none)` | 容器特征，不是 Linux 特征。`domainname` 是 `(none)` 因为**从没人调过 `setdomainname()`**（内核 `include/linux/uts.h:8-18` 里 `UTS_DOMAINNAME` 的默认值就是 `"(none)"`） |
