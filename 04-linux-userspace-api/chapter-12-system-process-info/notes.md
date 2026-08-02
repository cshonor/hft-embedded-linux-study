# TLPI 第 12 章 — System and Process Information

> 对应目录：`chapter-12-system-process-info/`  
> 书名原文：**System and Process Information**  
> ⚠️ **两条路径：** POSIX `uname()`（可移植）vs Linux `/proc`（功能全、不可移植）。

**优先级**：🔴（监控 / 调试 / 嵌入式与 HFT 观测常读 `/proc`）  
**前置**：[Ch11 System Limits](../chapter-11-system-limits/notes.md)（`sysconf` 的补充来源）  
**后置**：[Ch13 File I/O Buffering](../chapter-13-file-io-buffering/notes.md) · 读 `/proc` 依赖的 **open/read** 见 [Ch4](../chapter-04-file-io-universal/notes.md)  
**相关**：[Ch24 fork](../chapter-24-process-creation/notes.md) · [Ch35 调度](../chapter-35-process-priorities-scheduling/notes.md) · [Ch49 mmap / maps](../chapter-49-memory-mappings/notes.md)

---

## 章节目标

获取系统软硬件与进程运行信息；会用 `uname`；会解析 `/proc` 全局文件与 `/proc/[pid]`；分清可移植 API 与 Linux 专属接口；为进程管理、监控、调试打底。

---

## 12.1 `uname()` — POSIX

```c
#include <sys/utsname.h>
int uname(struct utsname *buf);
```

| 字段 | 含义 |
|------|------|
| `sysname` | OS 名（Linux） |
| `nodename` | 主机名 |
| `release` | 内核发布号（如 6.x.x） |
| `version` | 编译信息 |
| `machine` | 架构（`x86_64` / `aarch64`） |
| `domainname` | GNU 扩展：NIS 域名 |

> **可移植首选**；**拿不到** CPU 数、内存总量、进程列表。

Demo：[`code/uname_demo.c`](./code/uname_demo.c)

---

## 12.2 `/proc` — Linux 核心特色（非 POSIX）

虚拟文件系统：不占磁盘，内容由内核**实时生成**。  
`mount | grep proc`

| 类 | 路径 | 内容 |
|----|------|------|
| 全局 | `/proc/xxx` | 整机信息 |
| 进程 | `/proc/[pid]/` | 每进程一份；`/proc/self` = 当前进程 |

### 常用全局文件

| 路径 | 用途 |
|------|------|
| `/proc/meminfo` | MemTotal / MemFree / Buffers / Cached… |
| `/proc/cpuinfo` | CPU 型号、核、缓存 |
| `/proc/version` | 内核版本字符串 |
| `/proc/loadavg` | 1/5/15 负载、运行中任务、最近 PID |
| `/proc/sys/` | 可调内核参数（`sysctl`） |
| `/proc/sys/kernel/pid_max` | PID 上限 |
| `/proc/sys/fs/file-max` | 系统级文件句柄上限 |

### `/proc/[pid]/` 关键项

| 路径 | 用途 |
|------|------|
| `cmdline` | 启动命令；参数以 **`\0`** 分隔 |
| `status` | 易读：UID/GID、VmRSS、VmSize、线程数 |
| `stat` | 紧凑数字字段（`ps` 数据源之一） |
| `statm` | 内存页数概览 |
| `maps` | 虚拟地址区域（库 / 堆 / 栈 / mmap） |
| `fd/` | 已打开 fd 的符号链接 |
| `cwd` / `exe` | 工作目录 / 可执行文件（符号链接） |
| `environ` | 环境变量；`\0` 分隔 |

Demo：[`code/proc_self_status.c`](./code/proc_self_status.c) · [`code/mini_ps.c`](./code/mini_ps.c)

---

## 12.3 读 `/proc` 编程要点

1. 内容**动态变**，多次读可能不一致  
2. 行格式 / 字段顺序**无铁律**，跨内核要容错  
3. 权限：未必能读别人进程的部分文件  
4. 按**文本行**解析，勿当巨型二进制一次吞  
5. 进程会退出 → 打开时可能已 **ENOENT**  
6. **不要 `mmap` `/proc` 文件**（非普通磁盘文件）

---

## 12.4 `sysctl()` 系统调用

早期读写内核参数，对应 `/proc/sys`。  
> **新代码不推荐**；直接读写 `/proc/sys/...` 或用命令 `sysctl`。

---

## 12.5 主机名 / 域名

```c
int gethostname(char *name, size_t len);
int sethostname(const char *name, size_t len);   /* 特权 */

int getdomainname(char *name, size_t len);
int setdomainname(const char *name, size_t len); /* 特权 */
```

`gethostname` ≈ `uname().nodename`。

---

## 12.6 `sysinfo()` — Linux

```c
#include <sys/sysinfo.h>
struct sysinfo info;
int sysinfo(&info);
```

| 成员 | 含义 |
|------|------|
| `uptime` | 启动以来秒数 |
| `loads[3]` | 1/5/15 负载 |
| `totalram` / `freeram` / … | 内存（注意 **`mem_unit`**） |
| `totalswap` / `freeswap` | 交换区 |

> 老内核有溢出风险；现代监控优先 **`/proc/meminfo`**。  
> 字节数 ≈ 字段 × `mem_unit`。

---

## 12.7 `get_nprocs()` — GNU

```c
#include <sys/sysinfo.h>
int get_nprocs(void);       /* 当前可用 CPU */
int get_nprocs_conf(void);  /* 配置的 CPU 总数 */
```

便于多核检测（底层常读 `/proc`）。

---

## 12.8 可移植 vs Linux

| 方案 | API | 场景 |
|------|-----|------|
| **可移植** | `uname`、`gethostname` | 所有 UNIX |
| **Linux** | `/proc`、`sysinfo`、`get_nprocs` | 功能全；BSD/macOS 勿依赖 |

`ps` / `top` / `htop` **核心数据源 = `/proc`**。

---

## 12.9 易错考点

1. **`/proc/[pid]/stat`**：第 2 字段是 `(comm)`；进程名含 `)` 时简易解析会炸  
2. **`cmdline` / `environ`**：`\0` 分隔，不是空格/换行；打印时把 `\0` 换成空格  
3. 勿 `mmap` `/proc`  
4. **`sysinfo` 内存单位**看 `mem_unit`  
5. **容器**里 `/proc` 是命名空间视图，不是宿主机全局  

---

## `/proc` 速查（背诵）

| 要什么 | 看哪里 |
|--------|--------|
| 内核版本 | `uname` 或 `/proc/version` |
| 内存 | `/proc/meminfo`（或谨慎用 `sysinfo`） |
| CPU | `/proc/cpuinfo`、`get_nprocs` |
| 负载 | `/proc/loadavg` |
| 本进程状态 | `/proc/self/status` |
| 本进程映射 | `/proc/self/maps` |
| 本进程 fd | `/proc/self/fd/` |
| 调参 | `/proc/sys/...` + `sysctl` |

---

## 与前后章

| 章 | 关联 |
|----|------|
| Ch11 | `sysconf`；本章 `/proc` 给更细粒度 |
| Ch4 / Ch13 | 读 `/proc` = 普通文件 I/O + 缓冲细节 |
| Ch24 / Ch35 | 进程列表、调度观测 |
| Ch49 | `/proc/[pid]/maps` 对照地址空间 |

---

## 练习

1. `uname` 打印系统信息  
2. 解析 `/proc/self/status`（UID、VmRSS…）  
3. 遍历 `/proc` 数字目录 → 迷你 `ps`  
4. `loadavg` + `meminfo` 简易监控  
5. 枚举 `/proc/self/fd`  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 可移植：`uname`；详情：`/proc` |
| 2 | `/proc/self` = 当前进程 |
| 3 | `cmdline`/`environ` 用 `\0` 分隔 |
| 4 | `stat` 的 `(comm)` 难解析；优先 `status` |
| 5 | 勿 mmap `/proc`；进程可能已消失 |
| 6 | 容器内 `/proc` 是隔离视图 |

---

## 参考

- Kerrisk · TLPI Ch12  
- `man 2 uname` · `man 5 proc` · `man 2 sysinfo` · `man 7 sysctl`
