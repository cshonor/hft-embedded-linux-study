# TLPI 第 09 章 — Process Credentials

> 对应目录：`chapter-09-process-credentials/`  
> 书名原文：**Process Credentials**  
> ⚠️ **本章讲进程运行时凭证**；账户数据库见 [Ch8 Users and Groups](../chapter-08-users-and-groups/notes.md)。

> **编号说明：** 若大纲把「凭证 / SUID」标成 Ch28，那是与 Kerrisk 错位——本仓库按书：凭证 = **Ch9**；[Ch28](../chapter-28-process-creation-exec-detail/notes.md) = fork/exec 更细规则；调度 = [Ch35](../chapter-35-process-priorities-scheduling/notes.md)（Ch29 是线程导论）。

**优先级**：🔴（权限检查 / setuid / 安全铺垫）  
**前置**：[Ch8 Users and Groups](../chapter-08-users-and-groups/notes.md) · [Ch24 fork](../chapter-24-process-creation/notes.md) · [Ch27 exec](../chapter-27-program-execution/notes.md)  
**后置**：[Ch10 Times and Dates](../chapter-10-time/notes.md) · [Ch38 特权程序](../chapter-38-secure-privileged/notes.md) · [Ch39 Capabilities](../chapter-39-capabilities/notes.md)

---

## 章节目标

掌握进程持有的全套 UID/GID；理解 **set-user-ID / set-group-ID**；分清特权/非特权下各 `set*id` 规则；知道 Linux 独有的 filesystem ID；为 Ch38 安全特权程序、Ch39 capabilities 打底。

内核侧对应概念：`struct cred`（各 ID 存在进程凭据里）。

---

## Ch8 vs Ch9（再强调）

| | Ch8 | Ch9 |
|--|-----|-----|
| 焦点 | `/etc/passwd` 等**账户库** | 进程**当前** R/E/S/F + 补充组 |
| 典型 API | `getpwnam` / `crypt` | `getuid` / `seteuid` / `getresuid` |

---

## 9.1 五类凭证

### 1. Real UID / GID（RUID / RGID）

| | |
|--|--|
| 含义 | **真正属于谁**（登录者身份） |
| 继承 | `fork` 继承；`exec` **默认不变** |
| 用途 | 发送信号权限、资源归属等 |
| 获取 | `getuid()` / `getgid()` |

### 2. Effective UID / GID（EUID / EGID）

| | |
|--|--|
| 含义 | **权限检查用的 ID**（文件、IPC 等） |
| 特权 | **EUID == 0** → 特权进程（root 能力意义上的） |
| 获取 | `geteuid()` / `getegid()` |

### 3. Saved set-user-ID / set-group-ID（SUID / SGID）

| | |
|--|--|
| 用途 | 专为 **set-UID / set-GID 程序** |
| `exec` | 新 EUID/EGID **复制到 Saved-ID** |
| 作用 | 允许在「普通身份 ↔ 文件属主身份」间**多次切换** |

### 4. File-system UID / GID（FUID / FGID · Linux 专有）

| | |
|--|--|
| 历史 | 早期想把「文件权限」与「其它」检查拆开 |
| 现状 | 自 2.2 起 **默认恒等于 EUID/EGID** |
| 修改 | `setfsuid()` / `setfsgid()`（现代代码几乎不用） |
| 移植 | **POSIX 无此概念**，跨平台勿依赖 |

### 5. Supplementary group IDs（补充组）

| | |
|--|--|
| 含义 | 用户所属多个组；权限检查一并计入 |
| 获取 | `getgroups(size, list)` |
| 修改 | `setgroups()` — **仅特权进程** |

Demo：[`code/print_credentials.c`](./code/print_credentials.c)

---

## 9.2–9.3 Set-User-ID / Set-Group-ID

可执行文件权限位：

```bash
chmod u+s program   # set-user-ID
chmod g+s program   # set-group-ID
ls -l               # 常见显示 -rwsr-xr-x / -rwxr-sr-x
```

`execve` 加载该程序时：

| 变化 | 结果 |
|------|------|
| EUID | = 文件属主 UID（若 setuid 位开） |
| EGID | = 文件属主 GID（若 setgid 位开） |
| Saved-ID | ← 拷贝新的 EUID/EGID |
| RUID | **不变**（仍是启动者） |

经典例子：`passwd`（属主 root + setuid）  
普通用户运行 → **EUID=0** 可改 shadow；**RUID 仍是普通用户**。

> ⚠️ setuid 程序是高危攻击面 → Ch38 安全编写。

---

## 9.4 Saved-ID 升降权流程

典型：普通用户跑 root 属主 setuid 程序

```
初始:  RUID=1000  EUID=0     SUID=0
seteuid(1000):
       RUID=1000  EUID=1000  SUID=0     ← 临时降权
seteuid(0):
       RUID=1000  EUID=0     SUID=0     ← 靠 Saved-ID 再提权
```

若无 Saved-ID，降权后**无法**再回到 0。

### 永久丢弃特权（对外 SUID 程序推荐）

业务结束后不要保留可恢复 root：

```c
setuid(getuid());   /* 特权进程：R/E/S 全部钉成 RUID，无法再 seteuid(0) */
```

临时降权用 `seteuid(getuid())`（Saved 仍为 0，可再提权）——仅内部可信阶段使用。

Demo：[`code/seteuid_drop_restore.c`](./code/seteuid_drop_restore.c) · [`code/setuid_permanent_drop.c`](./code/setuid_permanent_drop.c)

---

## 9.5 获取凭证 API

```c
#include <unistd.h>
#include <sys/types.h>

uid_t getuid(void);      /* RUID */
uid_t geteuid(void);     /* EUID */
gid_t getgid(void);      /* RGID */
gid_t getegid(void);     /* EGID */

#define _GNU_SOURCE
#include <unistd.h>
int getresuid(uid_t *ruid, uid_t *euid, uid_t *suid);
int getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid);

int getgroups(int size, gid_t list[]);  /* size==0 时返回组个数 */
```

FUID 无标准 getter；现代可视为 = EUID（除非调用过 `setfsuid`）。

---

## 9.6–9.7 修改凭证（重难点）

> **特权进程 = EUID==0**（不是 RUID==0）。  
> 参数 **`-1`** = 该项不改（`setreuid` / `setresuid` 系）。

### 对比总表

| 调用 | 改什么 | 特权进程 (EUID=0) | 非特权（含 setuid 程序） |
|------|--------|-------------------|--------------------------|
| `setuid(uid)` | 视情况 | **R+E+S 全改为 uid** | 仅能改 **EUID** → `uid` 须为 RUID 或 Saved-UID |
| `seteuid(e)` | 仅 EUID | 任意合法 uid | 仅能设为 **RUID 或 Saved-UID** |
| `setreuid(r,e)` | R 与/或 E | 能力更宽（见 man） | 受 R/Saved 约束；规则细看 `man 2 setreuid` |
| `setresuid(r,e,s)` | R/E/S 各独立 | 任意组合（`-1` 跳过） | 每项只能是当前 R/E/S 之一等（见 man）；**Linux 扩展** |
| `setfsuid` / `setfsgid` | FUID/FGID | 见 man | 几乎不用 |
| `setgroups` | 补充组列表 | 可改 | **失败**（需特权） |

`setgid` / `setegid` / `setregid` / `setresgid` 与上表对称（UID→GID）。

### 为何有这么多接口？

历史：BSD → SUSv3 → Linux 扩展；能力递增，旧接口保留兼容。  
新代码在 Linux 上若需精细控制：优先想清要不要 **`seteuid`（临时）** 还是 **`setresuid`（一次钉死 R/E/S）**。

Demo：[`code/seteuid_drop_restore.c`](./code/seteuid_drop_restore.c)（需自行 `chmod u+s` + 属主 root 才能看全路径）

---

## 9.8 fork / exec 对凭证（必考）

| 行为 | ruid | euid | saved-set UID |
|------|------|------|---------------|
| **fork()** | 继承 | 继承 | 继承 |
| **exec 普通程序**（无 SUID 位） | 不变 | **不变** | ← 拷贝当前 euid |
| **exec SUID 程序** | 不变 | = 文件属主 | ← 拷贝**新** euid（exec 前旧 euid 已反映在切换逻辑中；见 man/书表） |

要点：

- **只有 exec 加载带 SUID 位的程序**才会把 euid 换成文件属主；**fork 永不改 UID**。  
- SUID 进程再 fork：子**继承 euid 特权**；子若再 exec **普通**程序，euid **不会自动掉回** ruid。  
- Linux 上 shebang 脚本的 SUID/SGID **无效**（防脚本提权）。  
- 补充组：`getgroups`/`setgroups`（仅 root 可改）；exec/SUID **不改**附属组列表。

文件访问判定（看 euid / egid+附属组，Linux 文件侧还有 fsuid≈euid）：属主位 → 组位 → other。

> 写安全代码时用 `getresuid` **打印验证**；边角见 `man 2 execve` / `man 7 credentials`。

---

## 9.9 易混考点

1. **特权进程 ≠ RUID=0**  
   判据：**EUID==0**。普通用户跑 setuid-root：`RUID≠0, EUID=0` → 仍是特权进程。

2. **`setuid` vs `seteuid`**  
   特权下 `setuid(uid)` 常把 R+E+S 一并钉死；`seteuid` 只动有效 ID（可配合 Saved 来回切）。

3. **Saved-ID**  
   支持 setuid 程序 **多次** 降权 / 提权；对外服务结束应用 `setuid(getuid())` 永久丢弃。

4. **环境变量**  
   SUID 程序须警惕 `LD_PRELOAD` / `LD_LIBRARY_PATH` 等劫持；见 Ch38。

5. **FUID**  
   现代默认=EUID；POSIX 无；别写进可移植逻辑。

---

## 练习方向

1. 打印 R/E/S + 补充组（本目录 Demo）  
2. setuid 二进制内 `seteuid` 降权再提权  
3. 对比特权 vs 非特权下 `setuid` vs `seteuid`  
4. `exec` 前后 `getresuid`，观察 Saved-ID  

---

## 与后续章节

| 章 | 关联 |
|----|------|
| Ch27 Program Execution | `execve` 再结合 setuid |
| Ch38 Secure Privileged Programs | 凭证误用 → 漏洞 |
| Ch39 Capabilities | 现代细粒度特权，弱化「EUID=0 一把梭」 |

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 权限检查看 **EUID/EGID**（+ 补充组） |
| 2 | 特权 ⇔ **EUID==0** |
| 3 | setuid 位：`exec` 后 EUID=文件属主，RUID 不变，Saved←EUID |
| 4 | Saved-ID：为了能 `seteuid` 来回切换 |
| 5 | `setuid`(特权) 改 R+E+S；非特权多只能动 E |
| 6 | FUID：Linux 遗留，默认=EUID，勿依赖 |

---

## 参考

- Kerrisk · TLPI Ch9 Process Credentials  
- `man 7 credentials` · `man 2 setresuid` · `man 2 seteuid` · `man 2 getresuid`
